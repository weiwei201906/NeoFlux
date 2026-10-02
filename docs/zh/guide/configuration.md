# 配置（gflags）

NeoFlux 使用 [gflags](https://github.com/gflags/gflags) 做运行时配置，用
[glog](https://github.com/google/glog) 记录日志。下列参数均为可选，在命令行传入：

```powershell
.\build\bin\neoflux_app.exe --target_fps=120 --logtostderr
```

## 渲染后端（编译期）

tgfx 渲染后端在 **CMake 配置期** 选定，而非运行期。它没有对应的 gflag：
要切换后端，只能用不同的 `-DNEOFLUX_BACKEND=...` 重新编译。

| 取值 | 平台 | 说明 |
|------|------|------|
| `gl`（默认） | 桌面 + 移动端 | 桌面通过 WGL / CGL / GLX，移动端通过 EGL 走 OpenGL。唯一经过完整测试的后端。 |
| `vulkan` | 桌面 | 需要 Vulkan SDK 和支持 Vulkan 的驱动；tgfx 以 shaderc 构建。 |
| `d3d12` | 仅 Windows | 需要 Windows SDK 的 D3D12 头文件。 |
| `metal` | 仅 macOS | 需要 Apple Metal 框架。 |

用想要的后端配置并编译：

```bash
cmake -B build -DNEOFLUX_BACKEND=vulkan
cmake --build build
```

所选后端会以 `NEOFLUX_BACKEND_*` 预定义宏的形式传入 C++，并编入二进制。

::: tip OpenGL 是唯一经过完整测试的后端
Vulkan、D3D12、Metal 还需要额外的系统依赖（shaderc、Vulkan SDK、Windows
SDK 的 D3D12 头文件、Apple Metal 框架等），且尚未在 CI 中跑过。除非你有
明确的实验需求，否则使用 `gl`。
:::

::: warning 这是编译期选择
没有运行时参数可以切换后端。如果想在 OpenGL 构建之后试试 Vulkan，需要重新
用 `-DNEOFLUX_BACKEND=vulkan` 跑 CMake 并重新编译。
:::

## 完整参数表

| 参数 | 类型 | 默认值 | 含义 |
|------|------|--------|------|
| `--target_fps` | `int32` | `60`（来自 `config::kDefaultTargetFps`） | 应用事件循环的目标帧率。 |
| `--render_queue_capacity` | `uint64` | `2048`（来自 `config::kDefaultRenderQueueCapacity`） | SPSC 渲染命令环形队列容量，内部向上取整为 2 的幂（`std::bit_ceil`）；保留一个槽位，可用命令数 = `capacity - 1`。 |
| `--render_queue_drop_log_max` | `int32` | `10` | 每个进程最多打印多少次"渲染队列已满、丢弃命令"警告。超过后静默统计，不再刷屏。仅在诊断背压时调大。 |
| `--verbose_logging` | `bool` | `false` | 开启 `VLOG(1)` 并把 INFO 日志镜像到 stderr。 |
| `--logtostderr` | `bool` | `false` | 开启后所有日志写到 stderr 而非文件。 |
| `--log_dir` | `string` | `"./logs"` | `.log` 文件目录（自动创建），仅在未开启 `--logtostderr` 时生效。 |
| `--media_source` | `string` | `"./assets/media/sample.mp4"` | `/media` 路由中演示 `MediaWidget` 的视频路径或 URL。 |

### `--target_fps`

设置 `EventLoop::SetTargetFps()`。循环在帧之间睡眠以满足帧率上限；输入事件与
`MarkFrameDirty()` 会提前唤醒。默认 `60`。

### `--render_queue_capacity`

配置 `RenderLayer` 构造的 `SpscRingQueue<RenderCommand>`。内部用 `std::bit_ceil`
向上取整为 2 的幂，使下标回绕可用按位与完成。因保留一个槽位区分满/空，请求
`2048` 实际最多存 `2047` 条在途命令。若生产者追上消费者，本帧多余命令被丢弃
（限频警告）。

::: warning 仅在出现”render command queue full”警告时再调大
队列越大越占内存、增加呈现延迟。默认 `2048` 对一般 widget 树已足够。
:::

### `--render_queue_drop_log_max`

渲染命令队列满载丢帧时，NeoFlux 会打一条 `WARNING` 日志。为避免在持续
背压下刷屏，每个进程最多打印前 `N` 条丢帧警告，之后的丢弃被静默计数。
默认 `10`。设为 `0` 可完全关闭丢帧警告。

### `--media_source`

传给 `/media` 路由上内置 demo `MediaWidget` 的路径或 URL。默认
`./sample.mp4`。这个参数仅用于让 quick-start 应用不用重编译就能指向测试
视频；真实应用自己构造 `MediaWidget` 并直接调 `SetSource()`。

### `--verbose_logging`

一个便捷开关。开启后 NeoFlux 打开 `VLOG(1)`（`FLAGS_v = 1`）并把 INFO 级日志镜像到
stderr。逐帧布局转储（`VLOG(1) << widgetname [x y w h]`）就靠它显示。

### `--logtostderr`

这是 glog 内建参数。当 glog 未与 gflags 集成构建时，NeoFlux 会在 gflags 解析之前
预扫描 `argv`，识别 `--logtostderr` / `--nologtostderr`（及 `-` 短横线形式）。默认关闭，
日志写入文件。

### `--log_dir`

同样是 glog 内建参数，由同一套 argv 预扫描处理。同时接受 `--log_dir=路径` 与
`--log_dir 路径` 两种写法。写文件时默认 `./logs`，自动建目录，并追加 `.log` 扩展名；
目录无法创建时静默回退到 stderr。

## 代码里打日志

```cpp
#include <glog/logging.h>

LOG(INFO) << "Application started";
LOG(WARNING) << "Font not found, using default";
LOG(ERROR) << "Rendering failed";
VLOG(1) << "Detailed per-frame debug info";  // 配 --verbose_logging 显示
```

::: tip 推荐的开发命令
本应用是 GUI 子系统二进制、没有控制台，运行时建议：

```powershell
.\build\bin\neoflux_app.exe --logtostderr --verbose_logging
```
:::

## 完整示例

```powershell
.\build\bin\neoflux_app.exe `
  --target_fps=144 `
  --render_queue_capacity=4096 `
  --logtostderr `
  --verbose_logging
```

即目标 144 FPS、队列扩到 4096，并把详细日志输出到终端。

## 编译时常量（`core/config.h`）

部分值是**编译期**的（编入二进制，不能运行时改）。它们定义在
`neoflux/include/neoflux/core/config.h` 中，为 `inline constexpr`：

| 常量 | 默认值 | 含义 |
|------|--------|------|
| `config::kCacheLineSize` | `64` | CPU 缓存行字节数。用于 SPSC 队列头尾分占不同缓存行（避免伪共享）。Apple M 系列/新 AMD 可用 `-DNEOFLUX_CACHE_LINE_SIZE=128` 覆盖。 |
| `config::kDefaultRenderQueueCapacity` | `2048` | `--render_queue_capacity` 的默认值。 |
| `config::kDefaultTargetFps` | `60` | `--target_fps` 的默认值。 |
| `config::kLongPressThresholdMs` | `500` | `Button` 长按检测阈值（毫秒）。 |
| `config::kFlingStopThreshold` | `0.02` | 惯性滑动停止阈值（屏幕高度/秒）。 |

::: tip gflag 默认值跟随 config.h
gflag 定义使用这些常量作为默认值。改 `config.h` 里的 `kDefaultTargetFps`
为 30 并重新编译，`--target_fps` 的默认值自动变成 30，无需修改 flag 定义。
:::
