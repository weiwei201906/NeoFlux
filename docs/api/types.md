# Common Types & Enums

Shared value types used across the widget, layout, and input layers.

Header: `<neoflux/core/types.h>`. Input enums: `<neoflux/renderers/glfw_bridge.h>`.

## Geometry

```cpp
struct Point  { float x = 0, y = 0; };
struct Size   { float width = 0, height = 0; };
struct Rect   { float x = 0, y = 0, width = 0, height = 0;
                float right() const;   // x + width
                float bottom() const; };  // y + height
```

`isize = std::ptrdiff_t` is the preferred signed size/count type.

## Color

```cpp
struct Color {
  uint8_t r = 0, g = 0, b = 0, a = 255;
  static Color FromArgb(uint32_t argb) noexcept;
  uint32_t ToArgb() const noexcept;
};
```

Channels are 0–255. The `{r, g, b, a}` aggregate form is used throughout:

```cpp
Color red{220, 40, 40, 255};
auto web = Color::FromArgb(0xFF2A2A2A);
```

## EdgeInsets / LayoutConstraints

```cpp
struct EdgeInsets { float left = 0, top = 0, right = 0, bottom = 0; };
struct LayoutConstraints {
  float min_width = 0, max_width = 0, min_height = 0, max_height = 0;
};
```

EdgeInsets supports designated initializers:

```cpp
container->SetPadding({.left = 16, .top = 8, .right = 16, .bottom = 8});
```

## Alignment & axis enums

```cpp
enum class HAlign { kLeft, kCenter, kRight };
enum class VAlign { kTop, kCenter, kBottom };
enum class Axis   { kHorizontal, kVertical };
```

`FlexDirection` (from `<neoflux/widgets/container.h>`):
`kRow`, `kRowReverse`, `kColumn`, `kColumnReverse`.

## WidgetState

```cpp
enum class WidgetState {
  kIdle, kHovering, kDragging, kLoading, kSuccess, kError, kDisabled,
};
```

Lightweight lifecycle state for state-machine-driven widgets. Subclasses override
the state-change hook to launch coroutine animations, etc.

## Input enums (GLFW bridge)

```cpp
enum class MouseButton { kLeft = 0, kRight = 1, kMiddle = 2 };
enum class InputAction { kPress = 1, kRelease = 0, kRepeat = 2 };
```

Callback typedefs: `InputEventCallback(button, action, pos)`,
`ScrollEventCallback(xoffset, yoffset)`, `ResizeCallback(w, h)`,
`MouseMoveCallback(pos)`.
