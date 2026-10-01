# Configuration (gflags)

NeoFlux uses [gflags](https://github.com/gflags/gflags) for runtime
configuration and [glog](https://github.com/google/glog) for logging. Every
flag below is optional and passed on the command line:

```powershell
.\build\bin\hello_neoflux.exe --render_backend=gl --target_fps=120 --logtostderr
```

## Full flag reference

| Flag | Type | Default | Meaning |
|------|------|---------|---------|
| `--render_backend` | `string` | `"vulkan"` | Selects the tgfx render backend. Accepted values: `vulkan`, `gl`, `cpu`. |
| `--target_fps` | `int32` | `60` | Target frames-per-second for the application event loop. |
| `--render_queue_capacity` | `uint64` | `2048` | Capacity of the SPSC render-command ring queue. Rounded up to a power of two; one slot is reserved, so usable commands = `capacity - 1`. |
| `--verbose_logging` | `bool` | `false` | Enables `VLOG(1)` output and mirrors INFO logs to stderr. |
| `--logtostderr` | `bool` | `false` | When set, writes all logs to stderr instead of files. |
| `--log_dir` | `string` | `"./logs"` | Directory for `.log` files (created automatically). Only used when `--logtostderr` is off. |

### `--render_backend`

Defined in `src/render/render_layer.cpp`. Three values are accepted:

| Value | Behavior |
|-------|----------|
| `vulkan` (default) | Reserved. Not yet implemented — logs a warning and falls back to OpenGL. |
| `gl` | OpenGL backend (the currently working desktop path, via GLFW/WGL). |
| `cpu` | Software rasterizer. Not yet implemented — logs a warning and falls back to OpenGL. |

Any unrecognised value also logs a warning and uses the OpenGL path.

::: tip
Even though the default string is `"vulkan"`, on today's desktop builds every
option effectively renders through GL. Pass `--render_backend=gl` to make
the intent explicit.
:::

### `--target_fps`

Sets `EventLoop::SetTargetFps()`. The loop sleeps between frames to respect
the cap; input events and `MarkFrameDirty()` wake it early. Default `60`.

### `--render_queue_capacity`

Configures the `SpscRingQueue<RenderCommand>` constructed by `RenderLayer`.
The value is rounded up to the next power of two internally. Because one slot
is reserved to distinguish full from empty, a requested `2048` holds at most
`2047` in-flight commands. If the producer catches up to the consumer, excess
commands for that frame are dropped (rate-limited warning).

::: warning Raise this only if you see "render command queue full" warnings
A larger queue costs memory and can add present latency. The default `2048`
is plenty for typical widget trees.
:::

### `--verbose_logging`

A convenience bool. When set, NeoFlux turns on `VLOG(1)` (`FLAGS_v = 1`) and
mirrors INFO-level logs to stderr. This is the switch that makes the
per-frame layout dump (`VLOG(1) << widgetname [x y w h]`) appear.

### `--logtostderr`

This is a glog built-in flag. When glog is built without gflags integration,
NeoFlux pre-scans `argv` for `--logtostderr` / `--nologtostderr` (and the
`-` short forms) before gflags parses the rest. By default it is off and logs
go to files.

### `--log_dir`

Also a glog built-in flag, handled by the same argv pre-scan. Accepts both
`--log_dir=PATH` and `--log_dir PATH`. When logging to files, it defaults to
`./logs`, creates the directory if needed, and appends the `.log` extension.
If the directory cannot be created, logging silently falls back to stderr.

## Logging in code

```cpp
#include <glog/logging.h>

LOG(INFO) << "Application started";
LOG(WARNING) << "Font not found, using default";
LOG(ERROR) << "Rendering failed";
VLOG(1) << "Detailed per-frame debug info";  // shown with --verbose_logging
```

::: tip Recommended development invocation
Because examples are built as GUI-subsystem binaries with no console, run
with:

```powershell
.\build\bin\hello_neoflux.exe --logtostderr --verbose_logging
```
:::

## Full example

```powershell
.\build\bin\hello_neoflux.exe `
  --target_fps=144 `
  --render_backend=gl `
  --render_queue_capacity=4096 `
  --logtostderr `
  --verbose_logging
```

This targets 144 FPS, forces the GL backend, enlarges the command queue to
4096, and streams verbose logs to the terminal.
