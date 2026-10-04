# Renderer / Render Backends

The render layer sits on its own thread and consumes render commands pushed from
the application thread over the SPSC ring queue. It replays the commands as
tgfx draws: a tgfx `Window` owns the GPU context and swapchain on every
platform, and `context->submit()` presents each frame. The concrete GPU backend
is tgfx's own compile-time `TGFX_USE_*` choice — there is no runtime backend
flag (see [GPU backend](../guide/configuration.md)).

## RenderLayer

```cpp
class RenderLayer;  // obtained via Application::GetRenderLayer()
```

You normally never construct it directly. `Application` owns it and exposes it
through:

```cpp
RenderLayer& Application::GetRenderLayer() noexcept;
```

It owns the tgfx device (through `TgfxRenderer`), the command sink, and
presents each frame.

## GlfwBridge (desktop window & input)

```cpp
class GlfwBridge : public NonCopyable;
```

Header: `<neoflux/renderers/glfw_bridge.h>`. Wraps the GLFW window (created
with `GLFW_NO_API`) and translates OS input into NeoFlux callbacks. It is a
pure window + input bridge: GPU context, surface, and presentation belong to
the tgfx `Window` inside `TgfxRenderer`.

| Method | Signature | Notes |
|--------|-----------|-------|
| `Init` | `bool Init(int w, int h, std::string_view title)` | create window (`GLFW_NO_API`). |
| `Shutdown` | `void Shutdown() noexcept` | |
| `PollEvents` | `void PollEvents() const` | non-blocking. |
| `ShouldClose` | `bool ShouldClose() const` | |
| `GetFramebufferSize` | `void GetFramebufferSize(int& w, int& h) const` | |
| `GetWindowSize` | `void GetWindowSize(int& w, int& h) const` | |
| `GetNativeHandle` | `GLFWwindow* GetNativeHandle() const noexcept` | handed to `TgfxRenderer::Init`. |
| `GetCursorPos` | `Point GetCursorPos() const noexcept` | |

Input callback setters: `SetInputCallback`, `SetScrollCallback`,
`SetResizeCallback`, `SetMouseMoveCallback`.

## gflag reference

All flags are parsed from `argc/argv` passed to `Application::Init`.

| Flag | Type | Default | Meaning |
|------|------|---------|---------|
| `--target_fps` | `int32` | `60` | Frame rate cap for the event loop. |
| `--render_queue_capacity` | `uint64` | `2048` | SPSC ring-queue slots. Rounded up to a power of two; usable slots = capacity - 1. |
| `--verbose_logging` | `bool` | `false` | Enable verbose (debug) logging. |
| `--logtostderr` | `bool` | `false` | Emit glog to stderr instead of files. |
| `--log_dir` | `string` | `"./logs"` | Directory for glog log files. |

### Example

```powershell
.\my_app.exe --target_fps=30 --verbose_logging --logtostderr
```

```bash
./my_app --render_queue_capacity=4096 --log_dir=./logs
```

::: tip Backend selection is compile-time
There is no runtime backend flag. Switching GPU backends means re-running
CMake with a different `TGFX_USE_*` switch and rebuilding — see
[GPU backend](../guide/configuration.md).
:::
