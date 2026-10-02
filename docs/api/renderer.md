# Renderer / Render Backends

The render layer sits on its own thread and consumes render commands pushed from
the application thread over the SPSC ring queue. On desktop it is the GLFW/WGL
bridge; on mobile it is tgfx. The concrete graphics backend is chosen at runtime
by the `--render_backend` flag.

## RenderLayer

```cpp
class RenderLayer;  // obtained via Application::GetRenderLayer()
```

You normally never construct it directly. `Application` owns it and exposes it
through:

```cpp
RenderLayer& Application::GetRenderLayer() noexcept;
```

It owns the GL/Vulkan context, the command sink, and presents each frame.

## GlfwBridge (desktop window & input)

```cpp
class GlfwBridge : public NonCopyable;
```

Header: `<neoflux/renderers/glfw_bridge.h>`. Wraps the GLFW window, the OpenGL
context, and translates OS input into NeoFlux callbacks.

| Method | Signature | Notes |
|--------|-----------|-------|
| `Init` | `bool Init(int w, int h, std::string_view title)` | create window. |
| `Shutdown` | `void Shutdown() noexcept` | |
| `PollEvents` | `void PollEvents() const` | non-blocking. |
| `SwapBuffers` | `void SwapBuffers()` | present. |
| `ShouldClose` | `bool ShouldClose() const` | |
| `GetFramebufferSize` | `void GetFramebufferSize(int& w, int& h) const` | |
| `GetWindowSize` | `void GetWindowSize(int& w, int& h) const` | |
| `GetNativeHandle` | `GLFWwindow* GetNativeHandle() const noexcept` | |
| `GetCursorPos` | `Point GetCursorPos() const noexcept` | |
| `GetGlContext` | `void* GetGlContext() const noexcept` | for tgfx. |
| `MakeContextCurrent` | `void MakeContextCurrent()` | |
| `ReleaseContext` | `static void ReleaseContext()` | detach. |

Input callback setters: `SetInputCallback`, `SetScrollCallback`,
`SetResizeCallback`, `SetMouseMoveCallback`.

## Full gflag reference

All flags are parsed from `argc/argv` passed to `Application::Init`.

| Flag | Type | Default | Meaning |
|------|------|---------|---------|
| `--render_backend` | `string` | `"gl"` | Graphics backend. Only `gl` is available in this build; `vulkan`, `cpu`, and any unknown value are a hard startup error (no silent fallback). |
| `--target_fps` | `int32` | `60` | Frame rate cap for the event loop. |
| `--render_queue_capacity` | `uint64` | `2048` | SPSC ring-queue slots. Rounded up to a power of two; usable slots = capacity - 1. |
| `--verbose_logging` | `bool` | `false` | Enable verbose (debug) logging. |
| `--logtostderr` | `bool` | `false` | Emit glog to stderr instead of files. |
| `--log_dir` | `string` | `"./logs"` | Directory for glog log files. |

### Example

```powershell
.\my_app.exe --render_backend=gl --target_fps=30 --verbose_logging --logtostderr
```

```bash
./my_app --render_backend=gl --render_queue_capacity=4096 --log_dir=./logs
```

::: warning No silent backend fallback
If the selected `--render_backend` cannot be created (e.g. `vulkan` in a
GL-only build, or any unknown value), NeoFlux logs an error and refuses to
start rather than silently falling back to GL. Pass `--render_backend=gl`.
:::
