# 配置（gflags）

NeoFlux 使用 [gflags](https://github.com/gflags/gflags) 做运行时配置，用
[glog](https://github.com/google/glog) 记录日志。下列参数均为可选，在命令行传入：

```powershell
.\build\bin\neoflux_app.exe --target_fps=120 --logtostderr
```

## 渲染后端（编译期）

GPU 后端是 tgfx 自己的编译期选择，不是 NeoFlux 的概念。用 tgfx 原生的
`TGFX_USE_*` CMake 开关选定（每次构建恰好一个）；`thirdparty/CMakeLists.txt`
按 tgfx 的固定优先级（VULKAN > D3D12 > METAL > OPENGL）消解，并导出唯一的
`TGFX_USE_*=1` 宏，保证 NeoFlux 源码与 tgfx 实际编译的后端一致。

| tgfx 开关 | 平台 | 说明 |
|------|------|------|
| `TGFX_USE_OPENGL`（Apple 之外的默认） | Linux / Windows / Android | Linux `tgfx::EGLWindow`（X11）、Windows `tgfx::WGLWindow`、Android `tgfx::EGLWindow`。唯一经过完整测试的后端。 |
| `TGFX_USE_VULKAN` | 桌面 | 需要 Vulkan SDK 和支持 Vulkan 的驱动；tgfx 以 shaderc 构建。 |
| `TGFX_USE_D3D12` | 仅 Windows | 需要 Windows SDK 的 D3D12 头文件。 |
| `TGFX_USE_METAL` | 仅 Apple | 需要 Apple Metal 框架。 |

用想要的后端配置并编译：

```bash
cmake -B build -DTGFX_USE_METAL=ON -DTGFX_USE_OPENGL=OFF
cmake --build build
```

::: tip OpenGL 是唯一经过完整测试的后端
Vulkan、D3D12、Metal 还需要额外的系统依赖（shaderc、Vulkan SDK、Windows
SDK 的 D3D12 头文件、Apple Metal 框架等），且尚未在 CI 中跑过。除非你有
明确的实验需求，否则保持 `TGFX_USE_OPENGL` 开启。
:::

::: warning 这是编译期选择
没有运行时参数可以切换后端。如果想在 OpenGL 构建之后试试 Vulkan，需要重新
用 `-DTGFX_USE_VULKAN=ON -DTGFX_USE_OPENGL=OFF` 跑 CMake 并重新编译。
:::

## 完整参数表

| 参数 | 类型 | 默认值 | 含义 |
|------|------|--------|------|
| `--target_fps` | `int32` | `60` | 应用事件循环的目标帧率。 |
| `--idle_fps` | `int32` | `15` | 空闲心跳帧率：连续数帧无渲染请求且无协程/定时器任务后，循环降到此帧率（输入事件立即唤醒）；`0` 表示禁用空闲降频。 |
| `--render_queue_capacity` | `uint64` | `2048` | SPSC 渲染命令环形队列容量，内部向上取整为 2 的幂（`std::bit_ceil`）；保留一个槽位，可用命令数 = `capacity - 1`。 |
| `--render_queue_drop_log_max` | `int32` | `10` | 每个进程最多打印多少次"渲染队列已满、丢弃命令"警告。超过后静默统计，不再刷屏。仅在诊断背压时调大。 |
| `--native_tuning` | `bool` | `true` | 平台原生调优层总开关；`false` 时所有入口均为 no-op。 |
| `--native_render_rt_priority` | `int32` | `1` | Linux/Android：渲染线程尝试的 SCHED_FIFO 优先级（1..99）。 |
| `--native_thread_nice` | `int32` | `-5` | Linux/Android：渲染线程（降级路径）与 UI 线程尝试的 nice 值（-20..19）。 |
| `--native_bigcore_threshold_permille` | `int32` | `950` | 大核判定阈值（相对最快核频率的千分比，500..1000）。 |
| `--native_mmcss_profile` | `string` | `"Games"` | Windows：渲染线程注册 MMCSS 使用的 profile。 |
| `--native_timer_period_ms` | `int32` | `1` | Windows：timeBeginPeriod 请求的定时器精度（毫秒，`0` 表示不请求）。 |
| `--verbose_logging` | `bool` | `false` | 开启 `VLOG(1)` 并把 INFO 日志镜像到 stderr。 |
| `--logtostderr` | `bool` | `false` | 开启后所有日志写到 stderr 而非文件。 |
| `--log_dir` | `string` | `"./logs"` | `.log` 文件目录（自动创建），仅在未开启 `--logtostderr` 时生效。 |

### `--idle_fps`

帧节拍有两个速率。只要应用还在产生工作（待恢复协程、定时器，或
`Application::MarkFrameDirty()` 发出的渲染请求），循环保持 `--target_fps`
全速；连续三帧无事可做后降到 `--idle_fps`，让空闲窗口的唤醒次数降到约
1/4；下一个渲染请求（任何输入事件）立即恢复全速。设 `--idle_fps=0`
可回到旧的恒速行为。

### 原生调优 flags

`--native_*` 配置平台调优层**尝试**什么（见
[平台原生调优层](./native-tuning)）。越界值会被钳制；OS 拒绝时一切尝试
依旧静默降级——这些 flag 只改变"多激进"，绝不改变正确性。
`--native_tuning=false` 是生产环境问题的单行逃生口。

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

这是唯一的编译期常量：其余一切可调项都是 gflags（见上方 flag 总表）。
控件行为参数（长按时长、惯性阈值）由各自控件持有，不进全局配置。

### 缓存行定尺寸与运行时检测

`config::kCacheLineSize` 是用于把 SPSC 渲染队列头/尾填充到不同缓存行的
**编译期**对齐值（以避免伪共享）。它被编入二进制，运行时无法更改——但硬件
缓存行未必是 64 字节。因此平台原生调优层会在启动时**校验**这一假设：
`neoflux::native::DetectCacheTopology()` 报告真实的一致性行大小（Linux/Android
读 sysfs、Windows 用 CPUID、Apple 用 `sysctl`），
`neoflux::native::VerifyCacheLineConfig()` 将其与 `config::kCacheLineSize`
比较，当硬件行比编入值更宽时**只告警一次**。

最典型的失配出现在 **Apple M 系列**：其 L1 数据缓存一致性行为 **128 字节**。
若构建停留在 64 字节默认值，队列头与尾各按 64 字节填充，相邻两个对象仍可能共占
同一条真实的 128 字节行，于是生产者与消费者持续在核间弹跳该行，伪共享并未因
填充而消除。若 `VerifyCacheLineConfig()` 打出该告警，请用检测到的尺寸重新构建：

```bash
# Apple Silicon（128 字节行）
cmake -B build -DNEOFLUX_CACHE_LINE_SIZE=128
cmake --build build
```

`-DNEOFLUX_CACHE_LINE_SIZE=N` 定义 `NEOFLUX_CACHE_LINE_SIZE` 宏，`config.h`
读取它并赋给 `kCacheLineSize`——同一个符号，只是更宽，无需改动源码。在 64 字节
机器上过度对齐无害但会浪费一点内存，因此请设为检测器报告的尺寸，而不要更大。
完整的平台矩阵与自动启动钩子见 [平台原生调优层](./native-tuning.md)。

::: tip 编译期 vs 运行时
只有必须为常量的值（如 `alignas` 实参）才放在 `config.h`。其余一切——
包括 `--target_fps`、`--render_queue_capacity` 的默认值——都直接定义在
`core/flags.cpp`，全部运行时可调。
:::
