# Drag & Drop

Demonstrates the `Draggable` widget with pointer events and the "state machine as condition lock" pattern. A colored box can be dragged around; a status label shows the current state (Idle/Hovering/Dragging) and offset.

## Running

```bash
./bin/drag_demo
```

## Key Concepts

### Draggable Widget

The `Draggable` widget extends `Container` and handles pointer events to track drag state. The drag offset is applied at paint time via `context.Translate()`, so Taitank layout is unaffected.

**Center-follows-cursor behavior:** The widget center always follows the mouse cursor. On pointer-down, the widget immediately jumps so its center is at the click point. During movement, `drag_offset_ += local_pos - bounds.size / 2` keeps the center on the cursor. This is achieved without `MarkNeedsBuild()` — only `MarkFrameDirty()` (called by event dispatch) triggers a repaint.

### Paint-Time Transform and Hit Testing

Because `Draggable` applies a paint-time translate, three mechanisms keep coordinates consistent:

1. `GetPaintOffset()` returns `drag_offset_` so event dispatch converts visual coords to local coords.
2. `HitTest()` subtracts `drag_offset_` before the base-class test, so clicks at the visual position hit correctly.
3. `DispatchPointerEvent/Move` subtract `GetPaintOffset()` when computing `local_pos`.

### State Machine

The widget transitions between three states:

- `kIdle` — no interaction
- `kHovering` — pointer over the widget
- `kDragging` — pointer pressed and moving

### Long-Press Detection

A long-press coroutine is launched on pointer-down. It uses a `weak_ptr` to the widget and checks the drag state after 500ms. If the pointer was released before the timeout, the coroutine observes the state change and returns silently. If held for 500ms+, a "[Long Press!]" indicator appears.

```cpp
Task<void> LongPressCoroutine(std::weak_ptr<DragBox> weak) {
  co_await Sleep(std::chrono::milliseconds(500));
  auto box = weak.lock();
  if (!box) co_return;          // widget destroyed
  if (!box->IsDragging()) co_return;  // released before timeout
  box->SetLongPressFired(true);
  box->MarkNeedsBuild();
}
```

This is the "state machine as condition lock" pattern: no explicit cancellation is needed — the state machine gates coroutine execution.

### Hit-Test Cache

Pointer-move events use a hit-test cache to avoid traversing the entire widget tree on every move. The cache is invalidated whenever layout changes.

## See Also

- [Draggable API](../api/draggable)
- [Input & Events Guide](../guide/input)
- [Coroutines Guide](../guide/coroutines)
