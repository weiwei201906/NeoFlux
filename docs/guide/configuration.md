# Configuration (gflags)

NeoFlux uses [gflags](https://github.com/gflags/gflags) for runtime
configuration and [glog](https://github.com/google/glog) for logging. Every
flag below is optional and passed on the command line:

```powershell
.\build\bin\neoflux_app.exe --target_fps=120 --logtostderr
```

## GPU backend (tgfx, compile-time)

The GPU backend is tgfx's own compile-time choice, not a NeoFlux concept.
Select it with tgfx's native `TGFX_USE_*` CMake switches (exactly one per
build); `thirdparty/CMakeLists.txt` resolves them with tgfx's own priority
(VULKAN > D3D12 > METAL > OPENGL) and exports a single `TGFX_USE_*=1` define
so NeoFlux sources agree with what tgfx compiled. tgfx is a **required**
dependency — configure fails if the `tgfx` target is missing.

| tgfx switch | Platform | Surface acquisition |
|-------|----------|---------------------|
| `TGFX_USE_OPENGL` (default outside Apple) | Linux / Windows / Android | Linux `tgfx::EGLWindow` (X11), Windows `tgfx::WGLWindow`, Android `tgfx::EGLWindow`. The only fully-tested backend. |
| `TGFX_USE_METAL` (Apple default) | Apple only | `tgfx::MetalWindow` over a `CAMetalLayer`. Apple + OpenGL is rejected at configure time. |
| `TGFX_USE_VULKAN` | Windows | `tgfx::VulkanWindow` (Win32 HWND). Requires a Vulkan-capable driver; tgfx is built with shaderc. |
| `TGFX_USE_D3D12` | Windows only | `tgfx::D3D12Window::MakeForHwnd`. Requires the Windows SDK D3D12 headers. |

Configure and build with the backend you want:

```bash
cmake -B build -DTGFX_USE_METAL=ON -DTGFX_USE_OPENGL=OFF
cmake --build build
```

::: tip OpenGL is the only fully-tested backend
Vulkan and D3D12 require additional system dependencies (shaderc, Vulkan
SDK, Windows SDK D3D12 headers) and are not yet exercised in CI. Keep
`TGFX_USE_OPENGL` on (outside Apple) unless you have a specific reason to
experiment.
:::

::: warning This is a compile-time choice
There is no runtime flag to switch backends. If you want to try Vulkan after
an OpenGL build, re-run CMake with `-DTGFX_USE_VULKAN=ON -DTGFX_USE_OPENGL=OFF`
and rebuild.
:::

## Full flag reference

| Flag | Type | Default | Meaning |
|------|------|---------|---------|
| `--target_fps` | `int32` | `60` | Target frames-per-second for the application event loop. |
| `--idle_fps` | `int32` | `15` | Idle heart-beat rate: after a few frames with no render request and no pending coroutine/timer work, the loop drops to this rate (input events wake it instantly). `0` disables idle throttling. |
| `--render_queue_capacity` | `uint64` | `2048` | Capacity of the SPSC render-command ring queue. Rounded up to a power of two (`std::bit_ceil`); one slot is reserved, so usable commands = `capacity - 1`. |
| `--render_queue_drop_log_max` | `int32` | `10` | Maximum number of "render command queue full, dropped commands" warnings emitted per process. After this many drops, subsequent overflows are counted silently. Raise this only when diagnosing back-pressure. |
| `--native_tuning` | `bool` | `true` | Master switch for the platform-native tuning layer; `false` makes every entry point a no-op. |
| `--native_render_rt_priority` | `int32` | `1` | Linux/Android: SCHED_FIFO priority attempted for the render thread (1..99). |
| `--native_thread_nice` | `int32` | `-5` | Linux/Android: nice value attempted for the render (fallback) and UI threads (-20..19). |
| `--native_bigcore_threshold_permille` | `int32` | `950` | Big-core frequency threshold in permille of the fastest core (500..1000). |
| `--native_mmcss_profile` | `string` | `"Games"` | Windows: MMCSS profile for the render thread registration. |
| `--native_timer_period_ms` | `int32` | `1` | Windows: timer resolution in ms requested via timeBeginPeriod (`0` = off). |
| `--verbose_logging` | `bool` | `false` | Enables `VLOG(1)` output and mirrors INFO logs to stderr. |
| `--logtostderr` | `bool` | `false` | When set, writes all logs to stderr instead of files. |
| `--log_dir` | `string` | `"./logs"` | Directory for `.log` files (created automatically). Only used when `--logtostderr` is off. |

### `--idle_fps`

Frame pacing has two rates. While the application keeps requesting work
(pending coroutines, timers, or render requests from `Application::MarkFrameDirty()`),
the loop runs at `--target_fps`. After three consecutive frames with nothing
to do, it drops to `--idle_fps` so an idle window costs ~4x fewer wake-ups;
the next render request (any input event) restores the full rate instantly.
Set `--idle_fps=0` to keep the legacy constant-rate loop.

### Native tuning flags

`--native_*` configure what the platform tuning layer *attempts* (see
[Native Tuning Layer](./native-tuning)). Out-of-range values are clamped and
every attempt still degrades silently when the OS refuses — the flags never
change correctness, only aggressiveness. `--native_tuning=false` is the
one-line escape hatch for production issues.

### `--target_fps`

Sets `EventLoop::SetTargetFps()`. The loop sleeps between frames to respect the
cap; input events and `MarkFrameDirty()` wake it early. Default `60`.

### `--render_queue_capacity`

Configures the `SpscRingQueue<RenderCommand>` constructed by `RenderLayer`.
The requested value is rounded up to the next power of two internally
(`std::bit_ceil`) so index wrapping can use a bitwise AND. Because one slot is
reserved to distinguish full from empty, a requested `2048` holds at most
`2047` in-flight commands. If the producer catches up to the consumer, excess
commands for that frame are dropped (rate-limited warning).

::: warning Raise this only if you see "render command queue full" warnings
A larger queue costs memory and can add present latency. The default `2048`
is plenty for typical widget trees.
:::

### `--render_queue_drop_log_max`

When the render command queue is full and commands are being dropped, NeoFlux
emits a `WARNING` log. To avoid flooding the log under sustained backpressure,
only the first `N` drop warnings per process are printed; later drops are
counted silently. Default `10`. Set to `0` to silence drop warnings entirely.

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
Because the app is built as a GUI-subsystem binary with no console, run with:

```powershell
.\build\bin\neoflux_app.exe --logtostderr --verbose_logging
```
:::

## Full example

```powershell
.\build\bin\neoflux_app.exe `
  --target_fps=144 `
  --render_queue_capacity=4096 `
  --logtostderr `
  --verbose_logging
```

This targets 144 FPS, enlarges the command queue to 4096, and streams verbose
logs to the terminal.

## Compile-time constants (`core/config.h`)

Some values are **compile-time** (baked into the binary, not runtime flags).
They live in `neoflux/include/neoflux/core/config.h` as `inline constexpr`:

| Constant | Default | Meaning |
|----------|---------|---------|
| `config::kCacheLineSize` | `64` | Cache line size in bytes. Used to pad SPSC queue head/tail to separate cache lines (prevents false sharing). Override with `-DNEOFLUX_CACHE_LINE_SIZE=128` for Apple M-series / newer AMD. |

This is the only compile-time constant: everything tunable at runtime is a
gflag (see the flag reference above). Widget behavior values (long-press
timing, fling thresholds) are owned by their widgets, not by global config.

### Cache-line sizing vs runtime detection

`config::kCacheLineSize` is the **compile-time** alignment used to pad the SPSC
render-queue head/tail onto separate cache lines (avoiding false sharing). It
bakes into the binary, so nothing at runtime can change it — but the hardware
line size is not necessarily 64. The native tuning layer therefore *verifies*
the assumption at startup: `neoflux::native::DetectCacheTopology()` reports the
real coherence line (from sysfs on Linux/Android, CPUID on Windows, `sysctl`
on Apple), and `neoflux::native::VerifyCacheLineConfig()` compares it against
`config::kCacheLineSize` and logs a **WARNING once** when the hardware line is
wider than the compiled-in value.

The classic mismatch is the **Apple M series**, whose L1 data-cache coherence
line is **128 bytes**. A build left at the 64-byte default pads the queue head
and tail to 64 bytes each; two adjacent objects can still share one real
128-byte line, so the producer and consumer keep bouncing that line between
cores and false sharing survives the padding. If `VerifyCacheLineConfig()` logs
that warning, rebuild with the detected size:

```bash
# Apple Silicon (128-byte line)
cmake -B build -DNEOFLUX_CACHE_LINE_SIZE=128
cmake --build build
```

`-DNEOFLUX_CACHE_LINE_SIZE=N` defines the `NEOFLUX_CACHE_LINE_SIZE` macro, which
`config.h` picks up and assigns to `kCacheLineSize` — the same symbol, just
wider, with no source change. Over-aligning on a 64-byte machine is harmless
but wastes a little memory, so set it to the size the detector reports, not
higher. For the full platform matrix and the automatic startup hook, see
[Native Tuning Layer](./native-tuning.md).

::: tip compile-time vs runtime
Only values that MUST be constants (e.g. `alignas` arguments) live in
`config.h`. Everything else — including `--target_fps` and
`--render_queue_capacity` defaults — is defined directly in
`core/flags.cpp` and tunable at runtime.
:::
