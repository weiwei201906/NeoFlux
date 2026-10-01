# 架构

NeoFlux 把一个 UI 应用拆成两个独立线程层，二者仅通过一条无锁队列通信。
应用层负责全部业务逻辑与布局；渲染层持有 GPU/GL 上下文，把录制好的命令变成像素。

## 两层划分

```
+------------------ 应用线程（Application::Run）-------------------+
|                                                                  |
|  EventLoop（--target_fps）                                       |
|    |  OnFrame()                                                 |
|    |   1. PollEvents() + ShouldClose()                          |
|    |   2. BuildDirtyWidgets()    重建脏的 StatefulWidget 子树     |
|    |   3. LayoutWidgetTree()     在 widget 树上跑 Taitank flexbox |
|    |   4. PaintAndSubmit()      录制 kBeginFrame .. draw* .. kEndFrame |
|    |                          |                                 |
|    |                          |  RenderLayer::Submit(commands, n) |
|    |                          v                                 |
|    |                SpscRingQueue<RenderCommand>  <-- 1 生产者 1 消费者 |
+----|--------------------------|-----------------------------------+
     |  frame_cv_.notify_one   |
+----|--------------------------v-----------------------------------+
|    |        渲染线程（RenderLayer::RenderLoop）                    |
|    |           等待 frame_cv_，排空队列，按 Begin/End 边界绘制      |
|    |                            |                                 |
|    |                            v                                 |
|    |                  TgfxRenderer（tgfx，或内置 GL 回退）         |
|    |                            |                                 |
|    |   桌面：GlfwBridge（GLFW 窗口 + WGL 上下文）-> SwapBuffers() |
|    |   移动：平台 Surface（ANativeWindow / CAMetalLayer）         |
+----+------------------------------------------------------------+
```

## 应用层（UI 线程）

运行在调用 `Application::Run()` 的线程上，是唯一允许触碰 widget 对象的线程。

- **Widget 树** —— `shared_ptr<Widget>` 组成的树。有状态 widget 重建子树；
  叶子 widget（`Text`、`Button`）上报自身固有尺寸。
- **Taitank 布局** —— 每个 `Widget` 持有一个不透明的 `taitank::TaitankNode`。
  `PerformLayout(w, h)` 对根节点跑 `taitank::DoLayout`，再把算好的矩形回写到 widget 树。
- **事件循环** —— `EventLoop` 中由条件变量驱动的循环，空闲时睡眠，被
  `WakeUp()`（输入、`MarkFrameDirty()`、定时器）唤醒。`--target_fps` 限制最高帧率。
- **命令录制** —— 在 `PaintAndSubmit()` 中，应用把 `kBeginFrame` 标记、各 widget
  的绘制命令、`kEndFrame` 标记依次追加进 `RenderContext`。

只有在确实“脏”的时候才产出一帧：若 `frame_dirty_` 与 widget 重建都没发生，
`OnFrame()` 直接返回，空闲 CPU 占用接近于零。

## 渲染层（渲染线程）

`RenderLayer::Start()` 派生一个专用 `std::thread` 运行 `RenderLoop()`。
该线程独占 GL 上下文。

- **GL 上下文归属** —— 桌面端上下文由 `GlfwBridge` 创建。先在主线程短暂 make
  current，让 OpenGL 加载器（`glfwGetProcAddress`/WGL）解析函数指针，然后 release，
  在渲染线程上重新 acquire，专门负责真正的绘制。
- **帧状态机** —— 只有 `kBeginFrame` 与 `kEndFrame` 之间的命令才会被执行，
  避免渲染线程在应用还在提交命令时就呈现半帧画面。
- **后端** —— `TgfxRenderer` 封装 `tgfx`。关闭 `NEOFLUX_USE_TGFX` 时回退到内置
  OpenGL 渲染器（shader + VBO + FreeType 字形图集）。`--render_backend` 可选
  `vulkan`、`gl`、`cpu`，未实现的选项打警告后使用 GL。

## 命令如何跨线程

两层不会跨帧直接互相调用渲染代码，而是：

1. **应用线程**是唯一生产者：`RenderLayer::Submit()` 对每条录制命令调用
   `command_queue_.TryPush(cmd)`。队列满时本帧多余命令被丢弃（限频警告）。
2. `Submit()` 随后置 `frame_ready_ = true` 并 `frame_cv_.notify_one()` 唤醒渲染线程。
3. **渲染线程**是唯一消费者：在 `frame_cv_` 上等待（最长 16ms），然后
   `TryPop()` 取走所有可用命令并分发给 `TgfxRenderer`。遇到 `kEndFrame` 时调用
   `EndFrame()`，桌面端再 `GlfwBridge::SwapBuffers()`。

### SpscRingQueue 细节

定义于 `include/neoflux/core/ring_queue.h`：

| 属性 | 值 |
|------|----|
| 类型 | 有界、**单生产者单消费者（SPSC）**、无锁 |
| 容量 | 由 `--render_queue_capacity` 设置（默认 `2048`），向上取整为 2 的幂 |
| 可用槽位 | `capacity - 1`（保留一个槽位区分满/空） |
| 索引 | `head_`（生产者）与 `tail_`（消费者）均 `alignas(64)`，避免缓存行伪共享 |
| 回绕 | 2 的幂容量使回绕可用 `& mask_` 代替 `%` |
| 接口 | 生产者 `TryPush()`，消费者 `TryPop()`，另提供 `Empty()`/`Full()`/`Size()` 快照 |

::: warning 仅允许一个生产者和一个消费者
该队列按 SPSC 安全设计，并非 MPMC。切勿从渲染线程 push，或从应用线程 pop。
`frame_cv_` 条件变量是唯一带互斥锁的握手；队列本身无锁等待。
:::

## 输入流

指针与滚动事件来自窗口线程的 `GlfwBridge`，交给 `Application`：

1. 把光标坐标从真实（DPI 缩放后的）窗口尺寸缩放到逻辑布局尺寸。
2. 递归 `HitTest()`（子节点按“从上到下”顺序测试），并带 hover 命中缓存，
   每次布局变化即失效。
3. 把按下/抬起交给消费了按下事件的 widget，把移动交给 hover（或按下）的 widget，
   滚动事件则**沿祖先冒泡**，直到某个 widget 消费它。

`pressed_widget_` 用 `weak_ptr` 持有，避免帧之间 widget 树重建造成悬垂指针。
完整事件契约见[输入与事件](./input)。

## 平台矩阵

| 平台 | 窗口 | GL/GL 上下文 | 渲染目标 |
|------|------|--------------|----------|
| Windows / Linux / macOS | `GlfwBridge`（GLFW） | WGL / GLX / CGL | 窗口 framebuffer |
| Android / iOS | 系统提供的 Surface | 无 | `ANativeWindow` / `CAMetalLayer` |

预处理器选择桥接实现：桌面构建 `glfw_bridge.cpp`，Android/iOS 构建
`mobile_bridge.cpp`（`NEOFLUX_PLATFORM_DESKTOP` 与 `NEOFLUX_PLATFORM_MOBILE`）。
