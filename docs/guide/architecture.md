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
- **Elements**: `RenderCommand` flat struct (draw rect, draw rounded rect, draw
  text, clip, translate, save/restore, begin/end frame)

The queue uses cache-line-aligned head/tail counters and `std::construct_at` /
`std::destroy_at` for safe element construction, supporting non-trivially
copyable types like `std::string`.

### Bitwise Index Wrapping

Capacity is rounded up to the next power of two (`std::bit_ceil`) so that
index wrapping uses a single bitwise AND instead of integer modulo:

```cpp
// In SpscRingQueue:
mask_ = capacity_ - 1;           // e.g. capacity=2048 -> mask=2047 (0x7FF)
next_head = (head + 1) & mask_;  // 1 cycle AND vs ~20-40 cycles for %
```

:::tip
This is why `--render_queue_capacity` is rounded up: any value becomes a
power of two, enabling the `& mask_` optimization. A capacity of 2048 stores
2047 elements (one slot is reserved for full/empty distinction).
:::

The same pattern applies elsewhere:
- Frame logging: `(frames_rendered & 63U) == 0U` instead of `% 60`
- Even/odd tests: `(x & 1U) == 0U` instead of `x % 2 == 0`
- Integer halving: `x >> 1` instead of `x / 2` (integers only)

## Thread Safety

- The widget tree is only accessed from the main thread.
- The render queue is the only shared data structure.
- The render thread only reads `RenderCommand` values from the queue; it never
  touches the widget tree.
- Coroutines are scheduled and resumed exclusively on the main thread by the
  `EventLoop`.

## Data Flow Deep Dive

This section traces a complete frame from user input to pixels on screen.

### 1. Input Reception (Platform Bridge)

```
GLFW mouse callback / Android touch event
        │
        ▼
PlatformBridge::input_callback_ (set by Application)
        │
        ▼
Application::DispatchPointerEvent(pos, action, button)
```

The platform bridge receives raw input events and forwards them to the
Application via a registered callback. On desktop, `GlfwBridge` converts GLFW
enum values to `PlatformBridge::InputAction` values (note: GLFW_RELEASE=0 but
`kRelease=1`, so a conversion function is required).

### 2. Hit Testing & Event Dispatch (Application Layer)

```
DispatchPointerEvent
        │
        ├─► HitTest(pos) ── recursive tree traversal ──► deepest widget
        │
        ├─► OnPointerDown/Up/Move/Enter/Exit(local_pos)
        │
        └─► MarkFrameDirty()  ◄── sets frame_dirty_ = true
```

`HitTest` walks the widget tree in reverse child order (top-most first),
checking each widget's `bounds_`. A hit-test cache avoids re-traversal when the
pointer moves but the layout hasn't changed. During an active drag, move events
bypass hit testing and go directly to the pressed widget.

### 3. Widget State Change & Dirty Marking

When a widget handles an event (e.g. `Button` changes press state, `Draggable`
updates offset), it calls `MarkNeedsBuild()` to schedule a rebuild. This sets a
dirty flag on the widget and propagates up to mark the frame dirty.

### 4. Frame Build (EventLoop)

When `frame_dirty_` is true, the next `EventLoop::Tick()` executes:

```
Tick()
  ├─► PollEvents()          ── platform bridge polls input
  ├─► RunReadyCoroutines()  ── 4-phase coroutine scheduler
  │     ├─ Promote yields (yield_handles_ → pending)
  │     ├─ Resume pending coroutines
  │     ├─ Collect completed (erase from active_tasks_)
  │     └─ Fire expired timers (lookup Task in active_tasks_ before resume)
  │
  ├─► Build dirty widgets   ── Build() for widgets with needs_build_ flag
  ├─► PerformLayout()       ── Taitank flexbox layout pass
  └─► Paint()               ── generate RenderCommands
```

### 5. Layout (Taitank)

`PerformLayout(width, height)` walks the widget tree:

1. Each `Container` creates/updates a Taitank flex node with its properties
   (flex direction, padding, margin, justify/align).
2. Leaf widgets (`Text`, `Button`) report intrinsic size via `OnMeasure()`.
3. `Taitank::DoLayout(root_node, width, height)` computes all positions/sizes.
4. `ReadLayoutRecursive()` copies computed bounds back into each widget's
   `bounds_`.

### 6. Paint & Command Generation

Each widget's `Paint(RenderContext&)` method emits `RenderCommand`s:

```
Widget::Paint()
  ├─► MakeSave()              ── push transform/clip state
  ├─► MakeTranslate(x, y)     ── move to widget position
  ├─► MakeClipRect(bounds)    ── clip to widget bounds
  ├─► MakeDrawRect/RoundedRect ── background
  ├─► MakeDrawText(text, ...) ── text rendering
  ├─► children->Paint()       ── recursive
  └─► MakeRestore()           ── pop state
```

Commands are pushed to the ring queue as they are generated. A frame is
delimited by `MakeBeginFrame()` and `MakeEndFrame()` commands.

### 7. SPSC Ring Queue Transfer

```
Application (producer)             Render (consumer)
       │                                  │
       │  TryPush(RenderCommand)          │
       │─────────────────────────────────►│
       │  (atomic head/tail, no lock)     │
       │                                  │  TryPop()
       │                                  │
```

The queue uses cache-line-aligned atomic head/tail counters to avoid false
sharing. `std::construct_at` places the element in raw storage; `std::destroy_at`
cleans up after pop. Capacity is rounded up to the next power of two for fast
modulo via bitwise AND.

### 8. Render Thread Consumption

The render thread runs `RenderLoop()`:

```
RenderLoop()
  ├─► Wait for frame signal (semaphore, blocks when no work)
  ├─► MakeContextCurrent()   ── platform_bridge_->MakeContextCurrent()
  ├─► Consume all commands in queue:
  │     ├─► BeginFrame       ── clear screen
  │     ├─► Save/Restore     ── transform/clip stack
  │     ├─► DrawRect         ── filled rectangle
  │     ├─► DrawRoundedRect  ── rounded rectangle (shader)
  │     ├─► DrawText         ── glyph rendering via FontManager + tgfx
  │     └─► EndFrame
  └─► SwapBuffers()          ── platform_bridge_->SwapBuffers()
```

On desktop, `GlfwBridge` provides the OpenGL context and window. On mobile,
`MobileBridge` provides the platform surface (ANativeWindow/CAMetalLayer) and
tgfx renders directly.

### 9. Presentation

`SwapBuffers()` presents the rendered frame to the window manager. The render
thread then waits on the semaphore for the next frame, consuming zero CPU while
idle.

## RenderCommand Types

| Command | Description |
|---------|-------------|
| `kNoop` | No operation (padding) |
| `kBeginFrame` | Start a new frame (clear buffer) |
| `kEndFrame` | End frame (flush) |
| `kDrawRect` | Filled axis-aligned rectangle |
| `kDrawRoundedRect` | Filled rectangle with rounded corners |
| `kDrawText` | Text string at position with font/size/color |
| `kSave` | Push transform/clip state |
| `kRestore` | Pop transform/clip state |
| `kTranslate` | Apply translation offset |
| `kClipRect` | Set clipping rectangle |

## Memory Safety

NeoFlux employs multiple layers of memory safety guarantees:

### RAII Everywhere

- All widget tree nodes use `std::shared_ptr<Widget>` with `std::enable_shared_from_this`.
- Taitank layout nodes are wrapped in `std::unique_ptr<TaitankNode, TaitankNodeDeleter>`.
- Font faces are owned by `FontEntry` (MappedMemory + FT_Face), ensuring the mapped
  font data outlives the FreeType face.
- Mapped memory is managed by `MappedMemory` RAII wrapper (mmap/VirtualAlloc +
  munmap/VirtualFree in destructor).

### Guard Pages

The SPSC ring queue storage is allocated via `mmap`/`VirtualAlloc` with a trailing
guard page (`PROT_NONE` / `PAGE_NOACCESS`). Any out-of-bounds write triggers an
immediate access fault instead of silently corrupting adjacent memory:

```cpp
// MappedMemory allocates usable_size + page_size, then mprotect's the last page.
storage_.emplace(capacity_ * sizeof(T), /*guard_page=*/true);
// Writing past the last element crashes here: SIGSEGV (POSIX) / AV (Windows)
```

### Weak Pointer Lifeguards

- `pressed_widget_` uses `std::weak_ptr` to avoid dangling references across
  frames. If a widget is destroyed between press and release, `lock()` returns
  nullptr and the release is safely ignored.
- `hovered_widget_` and `hit_cache_` similarly use `std::weak_ptr`.
- Long-press detection coroutines accept `std::weak_ptr<Button>` and check
  `lock()` before accessing the widget, preventing use-after-free.

### Integer Overflow Protection

`SpscRingQueue::Init()` clamps capacity to `SIZE_MAX / sizeof(T)` before
`std::bit_ceil`, preventing overflow in `capacity * sizeof(T)` storage allocation:

```cpp
constexpr std::size_t kMaxSafeCapacity =
    (std::numeric_limits<std::size_t>::max)() / sizeof(T);
const std::size_t safe_capacity =
    (capacity > kMaxSafeCapacity) ? kMaxSafeCapacity : capacity;
capacity_ = std::bit_ceil(safe_capacity);
```

## Memory-Mapped Optimizations

NeoFlux uses `mmap`/`MapViewOfFile` for three performance-critical paths:

### Ring Queue Storage

Allocated via anonymous `mmap(MAP_PRIVATE | MAP_ANONYMOUS)` with a guard page.
Bypasses the heap allocator for large queues and provides page-aligned storage.

### Font Files

Loaded via `MappedMemory::FromFile()` → `FT_New_Memory_Face(data, size)`. Avoids
FreeType's internal `fopen`/`fread` path; the OS page cache manages the bytes, and
only rasterized glyphs trigger page faults.

### Texture Upload

Glyph bitmaps are uploaded directly via `glTexSubImage2D` with
`GL_UNPACK_ALIGNMENT=1`. Each glyph is uploaded only once (cached in the
atlas), so the synchronous upload cost is amortized across all subsequent
frames. A PBO-based async path was removed because `glMapBufferRange`
returned `nullptr` on some drivers when PBO storage was not pre-allocated,
causing silent glyph upload failure and invisible text.
