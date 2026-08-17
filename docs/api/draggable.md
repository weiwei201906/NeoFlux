# Draggable

A container widget that can be dragged with pointer input. The drag offset is applied as a paint-time translate, so the Taitank layout is unaffected.

## Overview

`Draggable` extends `Container` and overrides pointer event handlers to track drag state. When the user presses and moves the pointer, the widget's visual position is offset by the drag delta. The layout position (as computed by Taitank) remains unchanged.

## State Machine

`Draggable` uses a lightweight state machine with three states:

| State | Description |
|-------|-------------|
| `kIdle` | Pointer is not interacting with the widget. |
| `kHovering` | Pointer is over the widget but not pressed. |
| `kDragging` | Pointer is pressed and moving the widget. |

State transitions are triggered by pointer events:

- `OnPointerEnter` → `kHovering` (if not dragging)
- `OnPointerExit` → `kIdle` (if not dragging)
- `OnPointerDown` → `kDragging`
- `OnPointerUp` → `kIdle`

## Usage

```cpp
#include <neoflux/widget/draggable.h>

auto box = std::make_shared<Draggable>();
box->SetWidth(100.0F)
   .SetHeight(100.0F)
   .SetBackgroundColor({.r = 51, .g = 128, .b = 230, .a = 255})
   .SetBorderRadius(12.0F);

auto label = std::make_shared<Text>("Drag Me");
box->AddChild(label);
```

## API Reference

### Methods

| Method | Description |
|--------|-------------|
| `GetDragOffset() -> Point` | Returns the current drag offset (x, y). |
| `IsDragging() -> bool` | Returns true if the widget is currently being dragged. |

### Overridden Events

| Event | Behavior |
|-------|----------|
| `OnPointerDown(pos)` | Records press position, sets dragging state. |
| `OnPointerUp(pos)` | Ends drag, returns to idle state. |
| `OnPointerMove(pos)` | Updates drag offset when dragging, marks needs build. |
| `OnPointerEnter()` | Transitions to hovering state. |
| `OnPointerExit()` | Transitions to idle state. |
| `Paint(ctx)` | Saves context, translates by drag offset, paints children, restores. |

## Long-Press Detection Pattern

Combine `Draggable` with coroutines for long-press detection. The state machine acts as a "condition lock": a coroutine launched on pointer-down checks the widget state after sleeping; if the state has changed (pointer released), the coroutine returns silently.

```cpp
class DragBox : public Draggable {
 public:
  bool OnPointerDown(const Point& local_pos) override {
    Draggable::OnPointerDown(local_pos);
    auto weak = std::weak_ptr<DragBox>(
        std::static_pointer_cast<DragBox>(shared_from_this()));
    app_->GetEventLoop().Schedule(LongPressCoroutine(weak));
    return true;
  }

 private:
  static Task<void> LongPressCoroutine(std::weak_ptr<DragBox> weak) {
    co_await Sleep(std::chrono::milliseconds(500));
    auto box = weak.lock();
    if (!box || !box->IsDragging()) co_return;
    // Long press detected.
  }
};
```

## See Also

- [Widget](./widget)
- [Container](./container)
- [ScrollView](./scroll-view)
