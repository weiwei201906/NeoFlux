# 架构

NeoFlux 采用两层架构，层间通过无锁环形队列通信。

## 架构图

```
┌──────────────────────────────────────────────────────┐
│  Application Layer (主线程)                           │
│                                                      │
│  ┌──────────┐  ┌──────────┐  ┌──────────────────┐   │
│  │ Widget   │  │ Taitank  │  │ EventLoop +      │   │
│  │ Tree     │→ │ Layout   │  │ Coroutines       │   │
│  └──────────┘  └──────────┘  └────────┬─────────┘   │
│                                       │              │
│                              RenderCommand           │
│                                       ▼              │
│  ┌──────────────────────────────────────────────┐   │
│  │  SPSC RingQueue (无锁, FIFO)                 │   │
│  └───────────────────────┬──────────────────────┘   │
└──────────────────────────┼──────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────┐
│  Render Layer (渲染线程)                             │
│                                                      │
│  ┌─────────────────┐    ┌──────────────────────┐    │
│  │ RenderCommand   │    │ tgfx (移动端)         │    │
│  │ Consumer        │───▶│ GLFW + OpenGL (桌面) │    │
│  └─────────────────┘    └──────────────────────┘    │
└──────────────────────────────────────────────────────┘
```

## Application 层

运行业务逻辑，构建 Widget 树，通过 Taitank flexbox 引擎计算布局，记录渲染命令。

核心组件：
- **Application**：框架入口，管理窗口、事件循环、路由栈
- **EventLoop**：信号量驱动的帧调度，协程调度器，定时器队列
- **RouteRegistry**：路由注册表，支持路由注册与导航栈
- **Widget Tree**：Widget 组成的 UI 树，Taitank 节点绑定

### 帧流水线

Application 层每帧按以下顺序执行：

1. **轮询事件** — 平台桥接输入事件分发到 Widget 树
2. **构建脏 Widget** — 标记了 `MarkNeedsBuild()` 的 Widget 被重建
3. **布局** — `Taitank::DoLayout` 计算 Widget 位置和大小
4. **绘制** — 每个 Widget 的 `Paint()` 方法生成 `RenderCommand`
5. **提交** — 命令推入 SPSC 环形队列

仅当 `frame_dirty_` 标志被设置时（输入事件、路由变更、`MarkFrameDirty()`）才处理帧，减少空闲 CPU 占用。

## Render 层

从 SPSC 环形队列消费命令，使用 tgfx 执行绘制。

- **桌面端**：通过 GLFW 创建窗口与 OpenGL 上下文，tgfx 渲染到 GL framebuffer
- **移动端**：tgfx 直接渲染到平台提供的 Surface（ANativeWindow / CAMetalLayer）

### 桌面端渲染（GLFW Bridge）

`GlfwBridge` 继承 `PlatformBridge`，封装：
- 窗口创建与调整大小
- 鼠标/键盘输入回调
- OpenGL 上下文管理
- 帧缓冲区大小查询

### 移动端渲染（tgfx）

`MobileBridge` 提供平台 Surface，tgfx 统一 2D 图形 API，底层对接平台原生图形 API（Vulkan/Metal/GLES）。

## SPSC 环形队列

无锁单生产者单消费者有界环形缓冲区：
- **生产者**：Application 层（主线程）
- **消费者**：Render 层（渲染线程）
- **容量**：`--render_queue_capacity` 配置（默认 2048），自动向上取整为 2 的幂
- **元素**：`RenderCommand` 扁平结构体（draw rect、draw rounded rect、draw text、clip、translate、save/restore、begin/end frame）

队列使用缓存行对齐的原子 head/tail 计数器避免伪共享，`std::construct_at` / `std::destroy_at` 安全构造/析构元素，支持 `std::string` 等非平凡可拷贝类型。位运算回绕（`index & (capacity - 1)`）替代取模。

### 位运算索引回绕

容量向上取整为 2 的幂（`std::bit_ceil`），使索引回绕使用单次位运算 AND 而非整数取模：

```cpp
// SpscRingQueue 中：
mask_ = capacity_ - 1;           // 例如 capacity=2048 -> mask=2047 (0x7FF)
next_head = (head + 1) & mask_;  // 1 周期 AND vs ~20-40 周期取模
```

:::tip
这就是 `--render_queue_capacity` 会向上取整的原因：任何值都会变成 2 的幂，从而启用 `& mask_` 优化。容量 2048 实际存储 2047 个元素（保留一个槽位用于满/空区分）。
:::

相同模式应用于其他地方：
- 帧率日志：`(frames_rendered & 63U) == 0U` 替代 `% 60`
- 奇偶判断：`(x & 1U) == 0U` 替代 `x % 2 == 0`
- 整数减半：`x >> 1` 替代 `x / 2`（仅限整数）

## 线程安全

- Widget 树仅在主线程访问
- 渲染队列是唯一的共享数据结构
- 渲染线程仅从队列读取 `RenderCommand`，不触碰 Widget 树
- 协程由 `EventLoop` 在主线程独占调度和恢复

## 数据走向深度解析

本节追踪从用户输入到屏幕像素的完整帧流程。

### 1. 输入接收（平台桥接）

```
GLFW 鼠标回调 / Android 触摸事件
        │
        ▼
PlatformBridge::input_callback_（由 Application 注册）
        │
        ▼
Application::DispatchPointerEvent(pos, action, button)
```

平台桥接接收原始输入事件，通过注册的回调转发给 Application。桌面端 `GlfwBridge` 将 GLFW 枚举值转换为 `PlatformBridge::InputAction`（注意：GLFW_RELEASE=0 而 `kRelease=1`，需转换函数）。

### 2. 命中测试与事件分发（Application 层）

```
DispatchPointerEvent
        │
        ├─► HitTest(pos) ── 递归树遍历 ──► 最深层 Widget
        │
        ├─► OnPointerDown/Up/Move/Enter/Exit(local_pos)
        │
        └─► MarkFrameDirty()  ◄── 设置 frame_dirty_ = true
```

`HitTest` 按逆序遍历子节点（最上层优先），检查每个 Widget 的 `bounds_`。命中测试缓存避免布局未变时指针移动的重复遍历。拖拽进行中，移动事件绕过命中测试直接发给 pressed Widget。

### 3. Widget 状态变更与脏标记

Widget 处理事件时（如 `Button` 改变按下状态、`Draggable` 更新偏移），调用 `MarkNeedsBuild()` 调度重建。这设置 Widget 的脏标志并向上传播标记帧脏。

### 4. 帧构建（EventLoop）

`frame_dirty_` 为 true 时，下一次 `EventLoop::Tick()` 执行：

```
Tick()
  ├─► PollEvents()          ── 平台桥接轮询输入
  ├─► RunReadyCoroutines()  ── 4 阶段协程调度
  │     ├─ 提升 yield（yield_handles_ → pending）
  │     ├─ 恢复 pending 协程
  │     ├─ 清理完成（从 active_tasks_ 擦除）
  │     └─ 触发到期定时器（resume 前从 active_tasks_ 查 Task）
  │
  ├─► 构建脏 Widget         ── 有 needs_build_ 标志的 Widget 调用 Build()
  ├─► PerformLayout()       ── Taitank flexbox 布局遍历
  └─► Paint()               ── 生成 RenderCommand
```

### 5. 布局（Taitank）

`PerformLayout(width, height)` 遍历 Widget 树：

1. 每个 `Container` 创建/更新 Taitank flex 节点，设置属性（flex direction、padding、margin、justify/align）
2. 叶子 Widget（`Text`、`Button`）通过 `OnMeasure()` 报告固有尺寸
3. `Taitank::DoLayout(root_node, width, height)` 计算所有位置/大小
4. `ReadLayoutRecursive()` 将计算出的边界复制回每个 Widget 的 `bounds_`

### 6. 绘制与命令生成

每个 Widget 的 `Paint(RenderContext&)` 方法发出 `RenderCommand`：

```
Widget::Paint()
  ├─► MakeSave()              ── 压入变换/裁剪状态
  ├─► MakeTranslate(x, y)     ── 移动到 Widget 位置
  ├─► MakeClipRect(bounds)    ── 裁剪到 Widget 边界
  ├─► MakeDrawRect/RoundedRect ── 背景
  ├─► MakeDrawText(text, ...) ── 文本渲染
  ├─► children->Paint()       ── 递归
  └─► MakeRestore()           ── 弹出状态
```

命令生成时即推入环形队列。一帧由 `MakeBeginFrame()` 和 `MakeEndFrame()` 命令界定。

### 7. SPSC 环形队列传输

```
Application (生产者)             Render (消费者)
       │                                  │
       │  TryPush(RenderCommand)          │
       │─────────────────────────────────►│
       │  (原子 head/tail，无锁)          │
       │                                  │  TryPop()
       │                                  │
```

队列使用缓存行对齐的原子 head/tail 计数器避免伪共享。`std::construct_at` 在原始存储中构造元素；`std::destroy_at` 在 pop 后清理。容量向上取整为 2 的幂，通过位运算 AND 实现快速取模。

### 8. 渲染线程消费

渲染线程运行 `RenderLoop()`：

```
RenderLoop()
  ├─► 等待帧信号（信号量，无任务时阻塞）
  ├─► MakeContextCurrent()   ── platform_bridge_->MakeContextCurrent()
  ├─► 消费队列中所有命令：
  │     ├─► BeginFrame       ── 清屏
  │     ├─► Save/Restore     ── 变换/裁剪栈
  │     ├─► DrawRect         ── 填充矩形
  │     ├─► DrawRoundedRect  ── 圆角矩形（着色器）
  │     ├─► DrawText         ── FontManager + tgfx 字形渲染
  │     └─► EndFrame
  └─► SwapBuffers()          ── platform_bridge_->SwapBuffers()
```

桌面端 `GlfwBridge` 提供 OpenGL 上下文和窗口。移动端 `MobileBridge` 提供平台 Surface（ANativeWindow/CAMetalLayer），tgfx 直接渲染。

### 9. 呈现

`SwapBuffers()` 将渲染好的帧呈现给窗口管理器。渲染线程随后在信号量上等待下一帧，空闲时 CPU 占用为零。

## RenderCommand 类型

| 命令 | 说明 |
|------|------|
| `kNoop` | 无操作（填充） |
| `kBeginFrame` | 开始新帧（清屏） |
| `kEndFrame` | 结束帧（刷新） |
| `kDrawRect` | 填充轴对齐矩形 |
| `kDrawRoundedRect` | 填充圆角矩形 |
| `kDrawText` | 指定位置、字体、字号、颜色的文本 |
| `kSave` | 压入变换/裁剪状态 |
| `kRestore` | 弹出变换/裁剪状态 |
| `kTranslate` | 应用平移偏移 |
| `kClipRect` | 设置裁剪矩形 |

## 下一步

- [Widget 系统](./widgets)
- [渲染管线](./rendering)
- [跨平台](./cross-platform)
