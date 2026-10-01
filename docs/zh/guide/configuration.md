# 配置（gflags）

NeoFlux 使用 [gflags](https://github.com/gflags/gflags) 做运行时配置，用
[glog](https://github.com/google/glog) 记录日志。下列参数均为可选，在命令行传入：

```powershell
.\build\bin\hello_neoflux.exe --render_backend=gl --target_fps=120 --logtostderr
```

## 完整参数表

| 参数 | 类型 | 默认值 | 含义 |
|------|------|--------|------|
| `--render_backend` | `string` | `"vulkan"` | 选择 tgfx 渲染后端，可选 `vulkan`、`gl`、`cpu`。 |
| `--target_fps` | `int32` | `60` | 应用事件循环的目标帧率。 |
| `--render_queue_capacity` | `uint64` | `2048` | SPSC 渲染命令环形队列容量，向上取整为 2 的幂；保留一个槽位，可用命令数 = `capacity - 1`。 |
| `--verbose_logging` | `bool` | `false` | 开启 `VLOG(1)` 并把 INFO 日志镜像到 stderr。 |
| `--logtostderr` | `bool` | `false` | 开启后所有日志写到 stderr 而非文件。 |
| `--log_dir` | `string` | `"./logs"` | `.log` 文件目录（自动创建），仅在未开启 `--logtostderr` 时生效。 |

### `--render_backend`

定义于 `src/render/render_layer.cpp`，接受三个值：

| 取值 | 行为 |
|------|------|
| `vulkan`（默认） | 预留。尚未实现 —— 打警告并回退到 OpenGL。 |
| `gl` | OpenGL 后端（当前可用的桌面路径，经 GLFW/WGL）。 |
| `cpu` | 软件光栅化。尚未实现 —— 打警告并回退到 OpenGL。 |

无法识别的值也会打警告并走 OpenGL。

::: tip
虽然默认字符串是 `"vulkan"`，但当前桌面构建的所有选项最终都经由 GL 渲染。
可显式传 `--render_backend=gl` 表明意图。
:::

### `--target_fps`

设置 `EventLoop::SetTargetFps()`。循环在帧之间睡眠以满足帧率上限；输入事件与
`MarkFrameDirty()` 会提前唤醒。默认 `60`。

### `--render_queue_capacity`

配置 `RenderLayer` 构造的 `SpscRingQueue<RenderCommand>`。内部向上取整为 2 的幂。
因保留一个槽位区分满/空，请求 `2048` 实际最多存 `2047` 条在途命令。若生产者追上
消费者，本帧多余命令被丢弃（限频警告）。

::: warning 仅在出现“render command queue full”警告时再调大
队列越大越占内存、增加呈现延迟。默认 `2048` 对一般 widget 树已足够。
:::

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
示例是 GUI 子系统二进制、没有控制台，运行时建议：

```powershell
.\build\bin\hello_neoflux.exe --logtostderr --verbose_logging
```
:::

## 完整示例

```powershell
.\build\bin\hello_neoflux.exe `
  --target_fps=144 `
  --render_backend=gl `
  --render_queue_capacity=4096 `
  --logtostderr `
  --verbose_logging
```

即目标 144 FPS、强制 GL 后端、队列扩到 4096，并把详细日志输出到终端。
