# Input & Events

NeoFlux handles pointer (mouse/touch) and scroll events through a unified
hit-testing and dispatch system.

## Event Flow

```
Platform (GLFW/tgfx)
       │
       ▼
GlfwBridge::MouseButtonCallback
       │
       ▼
Application::DispatchPointerEvent
       │  coordinate scaling (DPI)
       ▼
Widget::HitTest(point)  ── recursive, finds deepest widget
       │
       ▼
Widget::OnPointerDown(local_point)
       │
       ▼
Button::HandlePress  ──  sets is_pressed_, MarkNeedsBuild
```

## Hit Testing

`HitTest()` is a recursive method that finds the deepest widget under a given
point. It works in parent-relative coordinates:

1. Check if the point is within this widget's `bounds_`.
2. If not, return `nullptr`.
3. If yes, convert to local coordinates and recurse into children (reverse
   order, top-most first).
4. If a child hits, return it; otherwise return `this`.

```cpp
std::shared_ptr<Widget> hit = root->HitTest(point);
```

## Pointer Events

### Press

When a pointer button is pressed, `DispatchPointerEvent` performs a hit test and
calls `OnPointerDown()` on the hit widget. The widget returns `true` if it
consumes the event.

```cpp
bool MyWidget::OnPointerDown(const Point& local_pos) {
  if (ContainsPoint(local_pos)) {
    // handle press
    return true;  // consume
  }
  return false;  // bubble to parent
}
```

### Release

On release, the previously pressed widget (tracked via `std::weak_ptr`)
receives `OnPointerUp()`. If the release is within the widget's bounds and it
was pressed, the button's `on_pressed` callback is invoked.

```cpp
bool Button::HandleRelease(const Point& local_pos) {
  if (is_pressed_ && ContainsPoint(local_pos) && on_pressed_) {
    on_pressed_();
    is_pressed_ = false;
    MarkNeedsBuild();
    return true;
  }
  is_pressed_ = false;
  return false;
}
```

## Scroll Events

Scroll events (mouse wheel) bubble up the widget tree:

1. Hit-test to find the widget under the cursor.
2. Call `OnPointerScroll()` on it.
3. If it returns `false`, try the parent.
4. Continue until a widget consumes the event or the root is reached.

`ScrollView` consumes scroll events to pan its content:

```cpp
bool ScrollView::OnPointerScroll(const Point&, double xoffset, double yoffset) {
  scroll_x_ -= xoffset * kScrollSpeed;
  scroll_y_ -= yoffset * kScrollSpeed;
  ClampScroll();
  return true;  // consume
}
```

## DPI Coordinate Scaling

On Windows with DPI virtualization, the window's logical size may differ from
its framebuffer (physical) size. NeoFlux scales cursor coordinates from the
actual window size to the layout size:

```cpp
// In Application::DispatchPointerEvent:
pos.x = raw_pos.x * window_width_ / actual_width;
pos.y = raw_pos.y * window_height_ / actual_height;
```

This ensures hit-testing uses the same coordinate space as layout.

## Custom Event Handling

To handle events in a custom widget, override the relevant virtual functions:

```cpp
class MyWidget : public Widget {
 public:
  bool OnPointerDown(const Point& local_pos) override {
    LOG(INFO) << "Pressed at (" << local_pos.x << ", " << local_pos.y << ")";
    return true;
  }

  bool OnPointerUp(const Point& local_pos) override {
    LOG(INFO) << "Released at (" << local_pos.x << ", " << local_pos.y << ")";
    return true;
  }

  bool OnPointerScroll(const Point&, double x, double y) override {
    LOG(INFO) << "Scrolled (" << x << ", " << y << ")";
    return true;
  }
};
```
