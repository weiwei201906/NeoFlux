# 调试与调优

如何拿到 NeoFlux 的诊断信息、读懂日志、调节运行时行为。大多数开关是
gflag（运行时），少数在 `neoflux/include/neoflux/core/config.h`（编译期）。

## 日志基础

NeoFlux 使用 [glog](https://github.com/google/glog)。默认日志写到
`./logs/*.log`（目录不存在会自动创建）。桌面端是 GUI 子系统、没有控制台，
开发时通常要把日志镜像到 stderr：

```powershell
.\build\bin\neoflux_app.exe --logtostderr
```

::: tip 开发时永远带上 --logtostderr
不带它，GUI 程序根本不开控制台，你什么 `LOG(ERROR)` 都看不到。
`./logs/` 下的文件虽然在，但另开窗口 tail 不如直接看终端快。
:::

| 参数 | 默认 | 作用 |
| --- | --- | --- |
| `--logtostderr` | 关 | 所有日志镜像到 stderr。 |
| `--log_dir` | `./logs` | `.log` 文件目录。 |
| `--verbose_logging` | 关 | 打开 `VLOG(1)`（逐帧布局转储）。 |

### 该看什么

- `FontManager scanned <dir>: found N fonts` —— 确认字体被加载。
- `Render command queue full, dropped K commands` —— 背压，见下文。
- `mpv` 相关行（开启媒体时）—— 解码器/GPU 上传状态。

`VLOG(1)` 每帧为每个布局过的 widget 打一行。很吵，但定位几何错误最快：

```powershell
.\build\bin\neoflux_app.exe --logtostderr --verbose_logging
```

## 渲染队列背压

App 线程把 `RenderCommand` 投入单生产者单消费者环形队列，渲染线程消费。
如果生产者跑赢消费者（比如一帧构建几千个 widget），队列满了就会丢命令：

```
WARNING: Render command queue full, dropped 14 commands
```

诊断与调优：

| 参数 | 默认 | 作用 |
| --- | --- | --- |
| `--render_queue_capacity` | `2048` | 队列槽位数，向上取整为 2 的幂，保留一个槽位，可用 = `capacity - 1`。 |
| `--render_queue_drop_log_max` | `10` | 每个进程最多打印多少次"队列满"警告。主动查背压时调大。 |

::: warning 不要盲目调大队列
队列越大越占内存、增加呈现延迟。看到丢帧先查：是不是某个 widget
每帧都 rebuild（本该在状态变化时才 `MarkNeedsBuild`），或者 Paint 路径
里每帧分配内存。
:::

## 帧循环与唤醒

NeoFlux **不**忙轮询。事件循环在帧之间休眠，被以下事件唤醒：

- 输入事件（指针、按键）；
- widget 代码里调 `Application::MarkFrameDirty()`；
- mpv 帧回调（播放媒体时）。

`--target_fps` 限制上限；有事件提前唤醒时循环仍会提前醒来。

::: tip 为什么屏幕要动一下鼠标才刷新
你改了状态但忘了调 `MarkNeedsBuild()` / `MarkFrameDirty()`。持有可变状态
的 widget 必须自己标记脏；框架不会猜。
:::

## 编译期开关

`neoflux/include/neoflux/core/config.h` 集中放移植时可能要调的常量：

| 常量 | 默认 | 何时改 |
| --- | --- | --- |
| `config::kCacheLineSize` | `64` | 在 128 字节行的 CPU 上（部分 Apple M、新 AMD）定义 `NEOFLUX_CACHE_LINE_SIZE=128` 避免伪共享。 |
| `config::kDefaultRenderQueueCapacity` | `2048` | 会被 `--render_queue_capacity` gflag 覆盖。 |
| `config::kLongPressThresholdMs` | `500` | Button 长按阈值。 |

## 代码规范：别这么写

### 不要把 fluent builder 当控制流用

链式调用是为了让*声明式*构建读起来顺，不是 `if`/`for` 的替代品：

```cpp
// 不推荐：把条件埋在链里。
root->SetPadding(...).AddChild(title);
if (show_button) root->AddChild(btn);   // OK，但别写成：
root->AddChild(show_button ? btn : nullptr);  // 子节点是 nullptr 就是 bug。
```

::: warning Fluent 只用于一次性构建
每个 `SetXxx()` 返回 `*this` 供链式调用。不要在一行链里同时改局部状态；
把树的构建放到一个函数里、建好就 return。长链 `.AddChild().AddChild()`
没问题；按运行时状态分支的链不行。
:::

### 不要在 `Paint()` 里分配

`Paint()` 每个可见 widget 每帧调一次。在这里分配（`std::string`、
`std::vector`、`std::function`）会在热路径上触发分配器。几何和字符串
应该缓存到 widget 成员里，不要在 `Paint` 里 new。

### 不要在 widget 代码里碰 GL / mpv / 原生句柄

widget 的 `Paint()` 只发 `RenderCommand`。真正的 GL 在渲染线程执行。
从 widget 直接调 `gl*` 是跨线程违规，在非当前 OpenGL context 上会直接崩。

### 用 `std::shared_ptr<Widget>`，别用裸 `new`

widget 树由 `shared_ptr` 持有。用 `std::make_shared<T>()` 构造，不要把
裸 `T*` 存进另一个 widget —— 反向引用用 `std::weak_ptr`，widget 销毁时
回调不会留悬垂指针。

```cpp
// 好：lambda 以 shared_ptr 捕获子节点，button 自己持有它。
auto play_btn = std::make_shared<Button>("Play");
play_btn->SetOnPressed([media, play_btn]() { /* ... */ });

// 坏：在长生命周期回调里捕获 this 却没有 weak 守卫。
toggle->SetOnPressed([this] { state_ = !state_; });  // 仅当 widget 比回调
  // 活得久才安全；异步/协程回调请用 weak_ptr。
```

## 快速排查清单

1. 黑窗口？先 `--logtostderr` —— 多半是字体目录没找到，或者后端不可用。
2. 日志说"queue full"？查每帧 rebuild 的 widget；调 `--render_queue_capacity`
   只是创可贴。
3. 屏幕要动鼠标才刷新？你忘了标记树脏。
4. 跨线程崩了？你在 App 线程调了 GL/mpv（或反过来）。重读
   [媒体线程模型](./media.md#threading-model)。
5. 中日韩字符缺失？把 CJK 字体放进 `assets/fonts/`，或看[字体系统](./fonts.md)。
