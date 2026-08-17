# Architecture

NeoFlux uses a two-layer architecture that cleanly separates business logic
from rendering.

## Overview

```
┌──────────────────────────────────────────────────────┐
│  Application Layer (main thread)                     │
│                                                      │
│  ┌──────────┐  ┌──────────┐  ┌──────────────────┐   │
│  │ Widget   │  │ Taitank  │  │ EventLoop +      │   │
│  │ Tree     │→ │ Layout   │  │ Coroutines       │   │
│  └──────────┘  └──────────┘  └────────┬─────────┘   │
│                                       │              │
│                              RenderCommand           │
│                                       ▼              │
│  ┌──────────────────────────────────────────────┐   │
│  │  SPSC RingQueue (lock-free, FIFO)            │   │
│  └───────────────────────┬──────────────────────┘   │
└──────────────────────────┼──────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────┐
│  Render Layer (render thread)                       │
│                                                      │
│  ┌─────────────────┐    ┌──────────────────────┐    │
│  │ RenderCommand   │    │ tgfx (mobile)        │    │
│  │ Consumer        │───▶│ GLFW + OpenGL (desk) │    │
│  └─────────────────┘    └──────────────────────┘    │
└──────────────────────────────────────────────────────┘
```

## Application Layer

The application layer runs on the main thread and is responsible for:

- **Widget tree management**: Building, updating, and tearing down widget trees.
- **Taitank layout**: Running flexbox layout on the widget tree each frame.
- **Input dispatch**: Hit-testing and routing pointer/scroll events.
- **Event loop**: Processing events, driving coroutines, and triggering frames.
- **Render command generation**: Converting laid-out widgets into
  `RenderCommand` objects.

### Frame Pipeline

Each frame in the application layer follows this sequence:

1. **Poll events** — GLFW input events are dispatched to the widget tree.
2. **Build dirty widgets** — Widgets marked with `MarkNeedsBuild()` are rebuilt.
3. **Layout** — `Taitank::DoLayout` computes widget positions and sizes.
4. **Paint** — Each widget's `Paint()` method generates `RenderCommand`s.
5. **Submit** — Commands are pushed to the SPSC ring queue.

A frame is only processed when the `frame_dirty_` flag is set (by input events,
route changes, or `MarkFrameDirty()`), reducing idle CPU usage.

## Render Layer

The render layer runs on a dedicated thread and is responsible for:

- **Consuming commands** — Pulling `RenderCommand`s from the ring queue.
- **Drawing** — Executing draw calls via tgfx (mobile) or OpenGL (desktop).
- **Buffer swapping** — Presenting the rendered frame to the screen.

### Desktop Rendering (GLFW Bridge)

On desktop platforms, NeoFlux uses GLFW for window management and OpenGL 3.3
for rendering. The `GlfwBridge` class wraps GLFW calls and provides:

- Window creation and resizing
- Mouse/keyboard input callbacks
- OpenGL context management
- Framebuffer size queries

### Mobile Rendering (tgfx)

On mobile platforms, NeoFlux uses [tgfx](https://github.com/Tencent/tgfx) for
rendering. tgfx provides a unified 2D graphics API that backs onto the platform's
native graphics API (Vulkan/Metal/GLES).

## SPSC Ring Queue

Communication between layers uses a **single-producer, single-consumer**
lock-free ring queue:

- **Producer**: Application layer (main thread)
- **Consumer**: Render layer (render thread)
- **Capacity**: Configurable via `--render_queue_capacity` (default 2048)
- **Elements**: `RenderCommand` union (draw rect, draw text, clip, transform)

The queue uses cache-line-aligned head/tail counters and `std::construct_at` /
`std::destroy_at` for safe element construction, supporting non-trivially
copyable types like `std::string`.

## Thread Safety

- The widget tree is only accessed from the main thread.
- The render queue is the only shared data structure.
- `pressed_widget_` uses `std::weak_ptr` to avoid dangling references across
  frames.
- `frame_dirty_` is an `std::atomic<bool>` for lock-free dirty flag checks.
