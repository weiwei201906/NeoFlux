# Performance Guide

NeoFlux is designed for lightweight, high-performance UI rendering. This guide
covers optimization techniques and best practices.

## Architecture-Level Optimizations

### Paint-Time Transforms vs. Layout Changes

Widgets that move visually (e.g., `Draggable`, `ScrollView`) should use
paint-time transforms instead of changing layout properties:

```cpp
// GOOD: paint-time translate, no Taitank relayout
void Draggable::Paint(RenderContext& ctx) {
  ctx.Save();
  ctx.Translate(drag_offset_.x, drag_offset_.y);
  Container::Paint(ctx);
  ctx.Restore();
}

// BAD: changing layout properties triggers full Taitank relayout
void Draggable::OnPointerMove(const Point& p) {
  SetX(bounds_.x + p.x);  // triggers relayout of entire subtree
}
```

Paint-time transforms are $O(1)$ per frame; layout changes are $O(n)$ where $n$ is
the subtree size.

### Dirty Frame Semantics

Use the right dirty flag for the change:

| Change | Method | Cost |
|--------|--------|------|
| Widget tree structure | `MarkNeedsBuild()` | Build + Layout + Paint |
| Layout properties | `MarkFrameDirty()` | Layout + Paint |
| Paint-only (offset, scroll) | `MarkFrameDirty()` | Paint only (layout cached) |

:::tip
Pointer events automatically call `MarkFrameDirty()`. Do not call
`MarkNeedsBuild()` in `OnPointerMove()` for drag/scroll operations.
:::

### Hit-Test Cache

`Application` caches the last hit-test result for pointer-move events. The
cache is invalidated on layout changes. For large widget trees (>1000
widgets), this avoids a full tree traversal per mouse move.

## Bitwise Optimizations

NeoFlux uses bitwise operations throughout the hot paths:

### Ring Queue Index Wrapping

```cpp
mask_ = capacity_ - 1;            // power-of-two minus one
next_head = (head + 1) & mask_;   // 1 cycle vs ~20-40 for modulo
```

### Frame Rate Logging

```cpp
if ((frames_rendered & 63U) == 0U) {  // every 64 frames (2^6)
  LOG(INFO) << "Rendered " << frames_rendered << " frames";
}
```

### Even/Odd Tests

```cpp
if ((index & 1U) == 0U) {  // even index, faster than index % 2 == 0
  // ...
}
```

:::warning
Bitwise shifts (`>>`, `<<`) only apply to integer types. Floating-point
division (`/ 2.0F`) cannot use bitwise operations.
:::

## Memory Optimizations

### Minimal Types

Use the smallest type that fits:

```cpp
// Window dimensions never exceed 65535
std::uint16_t window_width_ = 800;
std::uint16_t window_height_ = 600;

// Enums with few values use uint8_t base type
enum class WidgetState : std::uint8_t { kIdle, kHovering, kDragging };
```

### Smart Pointers

- `std::unique_ptr` for exclusive ownership (renderer, platform bridge)
- `std::shared_ptr` for widget tree (shared ownership with parent/children)
- `std::weak_ptr` for cross-references (pressed widget, hovered widget,
  hit cache) to avoid reference cycles

### String Views

Use `std::string_view` for non-owning string parameters:

```cpp
void SetFontDir(std::string_view dir) noexcept;  // no copy
```

Only convert to `std::string` when ownership is needed (e.g., storing as a
member).

## Rendering Optimizations

### SPSC Ring Queue

The render queue is a lock-free single-producer single-consumer ring buffer:
- No mutex contention between main and render threads
- Cache-line-aligned head/tail counters avoid false sharing
- `std::construct_at`/`std::destroy_at` support non-trivial types

### Frame Batching

All render commands for a frame are batched between `kBeginFrame` and
`kEndFrame`. The render thread processes a complete frame per wake-up,
minimizing context switches.

### Semaphore-Blocked Render Thread

The render thread blocks on a semaphore when no frame is ready, consuming
zero CPU. It wakes only when the main thread submits a frame.

## Profiling

### Build with Debug Symbols

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

### Verbose Logging

```bash
./my_app --verbose_logging=true
```

Logs include frame counts, render queue state, and widget build cycles.

### Frame Rate

Check the log output for "Rendered N frames" — this prints every 64 frames.
Compare against the target FPS (`--target_fps`, default 60).

## Common Performance Pitfalls

1. **Calling `MarkNeedsBuild()` in `OnPointerMove()`** — triggers full widget
   rebuild per mouse event. Use paint-time transforms instead.
2. **Large widget trees without virtualization** — `ScrollView` does not yet
   support lazy widget creation. For lists >1000 items, consider manual
   widget recycling.
3. **Frequent font changes** — each `Text` widget with a different font
   triggers font loading. Reuse fonts across widgets.
4. **Blocking the main thread** — Long operations in event handlers or
   coroutines block frame processing. Use `co_await Yield()` to split work
   across frames.
