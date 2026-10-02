# Architecture

NeoFlux splits a UI application into two independently threaded layers that
communicate through a single lock-free queue. The application layer owns all
business logic and layout; the render layer owns the GPU/GL context and turns
recorded commands into pixels.

## Two-layer split

```
+----------------------- Application thread (Application::Run) ----------------------+
|                                                                                   |
|  EventLoop (--target_fps)                                                          |
|    |  OnFrame()                                                                   |
|    |   1. PollEvents() + ShouldClose()                                            |
|    |   2. BuildDirtyWidgets()    rebuild dirty StatefulWidget subtrees            |
|    |   3. LayoutWidgetTree()     Taitank flexbox over the widget tree             |
|    |   4. PaintAndSubmit()       record kBeginFrame .. draw* .. kEndFrame         |
|    |                          |                                                   |
|    |                          |  RenderLayer::Submit(commands, n)                 |
|    |                          v                                                   |
|    |                SpscRingQueue<RenderCommand>   <-- 1 producer, 1 consumer     |
+----|--------------------------|----------------------------------------------------+
     |  frame_cv_.notify_one   |
+----|--------------------------v----------------------------------------------------+
|    |              Render thread (RenderLayer::RenderLoop)                          |
|    |                 waits on frame_cv_, drains queue, honors Begin/End bounds     |
|    |                            |                                                 |
|    |                            v                                                 |
|    |                     TgfxRenderer  (tgfx, or built-in GL fallback)            |
|    |                            |                                                 |
|    |      desktop: GlfwBridge (GLFW window + WGL context) -> SwapBuffers()        |
|    |      mobile:  platform surface (ANativeWindow / CAMetalLayer)                 |
+----+-----------------------------------------------------------------------------+
```

## Application layer (UI thread)

Runs on the thread that calls `Application::Run()`. It is the only thread that
may touch widget objects.

- **Widget tree** — a `shared_ptr` tree of `Widget` nodes. Stateful widgets
  rebuild their subtree; leaf widgets (`Text`, `Button`) report intrinsic sizes.
- **Taitank layout** — every `Widget` owns an opaque `taitank::TaitankNode`.
  `PerformLayout(w, h)` runs `taitank::DoLayout` on the root and copies the
  computed rectangles back into the widgets.
- **Event loop** — a condition-variable-driven loop in `EventLoop`. It sleeps
  when idle and is woken by `WakeUp()` (input, `MarkFrameDirty()`, timers).
  `--target_fps` caps the maximum frame rate.
- **Command recording** — during `PaintAndSubmit()` the application appends a
  `kBeginFrame` marker, the widget paint calls (draw / clip / text commands),
  and a `kEndFrame` marker into a `RenderContext`.

A frame is only produced when something is actually dirty — `OnFrame()` early-
outs if neither `frame_dirty_` nor a widget rebuild occurred, which keeps idle
CPU near zero.

## Render layer (render thread)

`RenderLayer::Start()` spawns a dedicated `std::thread` running `RenderLoop()`.
This thread exclusively owns the GL context.

- **GL context ownership** — on desktop the context is created by the
  `GlfwBridge`. It is briefly made current on the main thread so the OpenGL
  loader (`glfwGetProcAddress` / WGL) can resolve function pointers, then
  released and re-acquired on the render thread for all actual drawing.
- **Frame state machine** — commands are only executed between `kBeginFrame`
  and `kEndFrame`. This prevents the render thread from presenting a partial
  frame while the application is still submitting commands.
- **Backend** — `TgfxRenderer` wraps `tgfx`. When `NEOFLUX_USE_TGFX` is off,
  the same class falls back to a built-in OpenGL renderer (shader + VBO +
  FreeType glyph atlas). The `--render_backend` flag defaults to `gl` (the only
  backend available in this build); `vulkan`, `cpu`, and unknown values are a
  hard startup error rather than a silent GL fallback.

## How commands cross threads

The two layers never call each other's rendering code across a frame. Instead:

1. The **application thread** is the sole producer: `RenderLayer::Submit()`
   calls `command_queue_.TryPush(cmd)` for every recorded command. If the
   queue is full the overflowing commands are dropped (rate-limited warning).
2. `Submit()` then sets `frame_ready_ = true` and `frame_cv_.notify_one()` to
   wake the render thread.
3. The **render thread** is the sole consumer: it waits on `frame_cv_` (16 ms
   max), then `TryPop()`s every available command and dispatches it to
   `TgfxRenderer`. At `kEndFrame` it calls `EndFrame()` and, on desktop,
   `GlfwBridge::SwapBuffers()`.

### SpscRingQueue details

Defined in `include/neoflux/core/ring_queue.h`:

| Property | Value |
|----------|-------|
| Kind | Bounded, **single-producer / single-consumer**, lock-free |
| Capacity | Set by `--render_queue_capacity` (default `2048`), rounded up to the next power of two |
| Usable slots | `capacity - 1` (one slot reserved to distinguish full from empty) |
| Indices | `head_` (producer) and `tail_` (consumer) are `alignas(64)` to avoid false sharing on cache-line boundaries |
| Wrap-around | Power-of-two size lets index wrapping use `& mask_` instead of `%` |
| API | `TryPush()` (producer), `TryPop()` (consumer), plus `Empty()` / `Full()` / `Size()` snapshots |

::: warning Only one producer and one consumer
The queue is intentionally SPSC-safe, not MPMC. Never push to it from the
render thread or pop from the application thread. The `frame_cv_` condition
variable is the only mutex-protected hand-shake; the queue itself is wait-free.
:::

## Input flow

Pointer and scroll events originate on the window thread via the `GlfwBridge`
and are delivered to `Application`, which:

1. Scales cursor coordinates from the real (DPI-scaled) window size to the
   logical layout size.
2. Runs a recursive `HitTest()` (children tested top-most first), with a
   hover hit-cache that is invalidated on every layout change.
3. Routes press/release to the widget that consumed the press, move events to
   the hovered (or pressed) widget, and lets scroll events **bubble up**
   ancestors until one consumes them.

`pressed_widget_` is held as a `weak_ptr` so a widget-tree rebuild between
press and release cannot leave a dangling pointer. See
[Input & Events](./input) for the full event contract.

## Platform matrix

| Platform | Windowing | GL/GL context | Render target |
|----------|-----------|----------------|---------------|
| Windows / Linux / macOS | `GlfwBridge` (GLFW) | WGL / GLX / CGL via GLFW | window framebuffer |
| Android / iOS | OS-provided surface | n/a | `ANativeWindow` / `CAMetalLayer` |

The preprocessor selects the bridge: `glfw_bridge.cpp` is built on desktop,
`mobile_bridge.cpp` on Android/iOS (`NEOFLUX_PLATFORM_DESKTOP` vs
`NEOFLUX_PLATFORM_MOBILE`).
