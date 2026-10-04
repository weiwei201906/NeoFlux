# Rendering Pipeline

NeoFlux uses a command-based rendering pipeline. The application layer generates
`RenderCommand` objects, and the render layer consumes and executes them.

## RenderCommand

`RenderCommand` is a tagged union that represents a single drawing operation:

| Type | Description |
|------|-------------|
| `kBeginFrame` | Start a new frame (clear, DPI scale) |
| `kEndFrame` | End frame (submit + present through the tgfx Window) |
| `kDrawRect` | Draw a filled rectangle |
| `kDrawRoundedRect` | Draw a filled rounded rectangle |
| `kDrawText` | Draw text glyphs |
| `kTranslate` | Push a translate transform |
| `kClipRect` | Set a clip rectangle |
| `kRestore` | Pop transform/clip state |

Commands are created via factory functions:

```cpp
RenderCommand cmd = RenderCommand::MakeDrawRect(rect, color);
RenderCommand text_cmd = RenderCommand::MakeDrawText(x, y, text, font, color);
```

## SPSC Ring Queue

Commands are passed from the application layer to the render layer via a
lock-free SPSC ring queue:

```cpp
// Application layer (producer):
render_layer_->Submit(cmd);

// Render layer (consumer):
while (running_) {
  RenderCommand cmd;
  while (queue_.TryPop(cmd)) {
    Execute(cmd);
  }
  WaitForNextFrame();
}
```

The queue capacity is configurable via `--render_queue_capacity` (default 2048).

## Desktop Rendering (tgfx)

Desktop and mobile share one rendering shape: a tgfx `Window` owns the GPU
context and the swapchain, `Surface::MakeFrom(context, window)` acquires a
per-frame surface, and `context->submit()` presents. Only window *creation*
is platform-specific:

| Backend | tgfx Window | Native handle source |
|---------|-------------|----------------------|
| `TGFX_USE_OPENGL` (Linux) | `tgfx::EGLWindow::MakeFrom(XID)` | `glfwGetX11Window()` |
| `TGFX_USE_OPENGL` (Windows) | `tgfx::WGLWindow::MakeFrom(HWND)` | `glfwGetWin32Window()` |
| `TGFX_USE_METAL` (Apple) | `tgfx::MetalWindow::MakeFrom(CAMetalLayer*)` | GLFW `NSWindow` content view |
| `TGFX_USE_VULKAN` (Windows) | `tgfx::VulkanWindow::MakeFrom(HWND)` | `glfwGetWin32Window()` |
| `TGFX_USE_D3D12` (Windows) | `tgfx::D3D12Window::MakeForHwnd(HWND)` | `glfwGetWin32Window()` |
| Android | `tgfx::EGLWindow::MakeFrom(ANativeWindow*)` | app shell |

GLFW windows are created with `GLFW_NO_API`; NeoFlux never manages a GL/EGL/WGL
context itself. There is no built-in rasterizer behind tgfx — tgfx is the only
rendering implementation.

## Text Rendering (tgfx)

Text is drawn by `TgfxRenderer::Execute` with `canvas->drawSimpleText` using
typefaces loaded by `FontManager` (FreeType-backed inside tgfx):

1. `FontManager` scans `assets/fonts/` and picks a default typeface.
2. `TgfxRenderer::Init` loads it via `tgfx::Typeface::MakeFromPath`.
3. Text commands create a sized `tgfx::Font` and draw directly on the canvas;
   tgfx handles glyph rasterization, caching, and subpixel positioning.

## Frame Synchronization

The render thread waits for commands using a condition variable:

```cpp
// Application layer signals new frame:
frame_cv_.notify_one();

// Render thread waits:
std::unique_lock lock(frame_mutex_);
frame_cv_.wait(lock, [this] { return has_commands_ || !running_; });
```

This minimizes idle CPU usage when no rendering is needed.
## Frame Synchronization

The render thread waits for commands using a condition variable:

```cpp
// Application layer signals new frame:
frame_cv_.notify_one();

// Render thread waits:
std::unique_lock lock(frame_mutex_);
frame_cv_.wait(lock, [this] { return has_commands_ || !running_; });
```

This minimizes idle CPU usage when no rendering is needed.
