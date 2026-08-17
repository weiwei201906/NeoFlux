# Widget

Abstract base class for all UI elements.

## Header

```cpp
#include <neoflux/widget/widget.h>
```

## Public Methods

### `GetWidgetName()`

```cpp
[[nodiscard]] virtual std::string_view GetWidgetName() const noexcept = 0;
```

Returns the widget's name for debugging and logging.

### `AddChild()`

```cpp
void AddChild(std::shared_ptr<Widget> child);
```

Adds a child widget.

### `ClearChildren()`

```cpp
void ClearChildren();
```

Removes all children.

### `GetChildren()`

```cpp
[[nodiscard]] const std::vector<std::shared_ptr<Widget>>& GetChildren() const noexcept;
```

Returns the list of children.

### `GetParent()`

```cpp
[[nodiscard]] Widget* GetParent() const noexcept;
```

Returns the parent widget, or `nullptr` if this is the root.

### `GetBounds()`

```cpp
[[nodiscard]] const Rect& GetBounds() const noexcept;
```

Returns the widget's computed bounds (position and size) after layout.

### `GetGlobalPosition()`

```cpp
[[nodiscard]] Point GetGlobalPosition() const;
```

Returns the widget's position in window coordinates (recursively sums parent
offsets).

### `PerformLayout()`

```cpp
void PerformLayout(float width, float height);
```

Runs Taitank layout on this widget's subtree. The root widget is sized to
`width x height`.

### `HitTest()`

```cpp
std::shared_ptr<Widget> HitTest(const Point& parent_pos);
```

Recursively finds the deepest widget under the given point. Returns `nullptr`
if no widget is hit.

### `MarkNeedsBuild()`

```cpp
void MarkNeedsBuild() noexcept;
```

Marks this widget for rebuild on the next frame.

### `NeedsBuild()` / `ClearNeedsBuild()`

```cpp
[[nodiscard]] bool NeedsBuild() const noexcept;
void ClearNeedsBuild() noexcept;
```

Query and clear the build-dirty flag.

### `GetTaitankNode()`

```cpp
[[nodiscard]] taitank::TaitankNode* GetTaitankNode() const noexcept;
```

Returns the underlying Taitank layout node.

## Virtual Methods (Override in Subclasses)

### `Build()`

```cpp
virtual std::shared_ptr<Widget> Build(BuildContext& context);
```

Returns the child widget tree. Called for dirty widgets during the build phase.

### `OnMeasure()`

```cpp
[[nodiscard]] virtual Size OnMeasure(float width, int width_mode,
                                     float height, int height_mode) = 0;
```

Returns the widget's intrinsic size. Leaf widgets must override this.

### `Paint()`

```cpp
virtual void Paint(RenderContext& context) = 0;
```

Generates render commands for this widget.

### `OnPointerDown()` / `OnPointerUp()`

```cpp
virtual bool OnPointerDown(const Point& local_pos);
virtual bool OnPointerUp(const Point& local_pos);
```

Handle pointer press/release events. Return `true` to consume the event.

### `OnPointerScroll()`

```cpp
virtual bool OnPointerScroll(const Point& local_pos, double xoffset,
                             double yoffset);
```

Handle scroll events. Return `true` to consume.

### `ReadLayoutRecursive()`

```cpp
virtual void ReadLayoutRecursive();
```

Copies Taitank layout results into `bounds_`. Override for widgets that need
custom post-layout processing (e.g., `ScrollView` computes content size).
