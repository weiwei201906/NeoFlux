# 公共类型与枚举

widget、布局与输入层共用的值类型。

头文件：`<neoflux/core/types.h>`。输入枚举：`<neoflux/renderers/glfw_bridge.h>`。

## 几何类型

```cpp
struct Point  { float x = 0, y = 0; };
struct Size   { float width = 0, height = 0; };
struct Rect   { float x = 0, y = 0, width = 0, height = 0;
                float right() const;   // x + width
                float bottom() const; };  // y + height
```

`isize = std::ptrdiff_t` 是推荐的有符号尺寸/计数类型。

## Color 颜色

```cpp
struct Color {
  uint8_t r = 0, g = 0, b = 0, a = 255;
  static Color FromArgb(uint32_t argb) noexcept;
  uint32_t ToArgb() const noexcept;
};
```

通道取值 0–255。代码中普遍使用 `{r, g, b, a}` 聚合形式：

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

EdgeInsets 支持指定初始化器：

```cpp
container->SetPadding({.left = 16, .top = 8, .right = 16, .bottom = 8});
```

## 对齐与轴向枚举

```cpp
enum class HAlign { kLeft, kCenter, kRight };
enum class VAlign { kTop, kCenter, kBottom };
enum class Axis   { kHorizontal, kVertical };
```

`FlexDirection`（来自 `<neoflux/widgets/container.h>`）：
`kRow`、`kRowReverse`、`kColumn`、`kColumnReverse`。

## WidgetState 组件状态

```cpp
enum class WidgetState {
  kIdle, kHovering, kDragging, kLoading, kSuccess, kError, kDisabled,
};
```

供状态机驱动的 widget 使用的轻量生命周期状态。子类可覆盖状态变更钩子以启动
协程动画等。

## 输入枚举（GLFW 桥接）

```cpp
enum class MouseButton { kLeft = 0, kRight = 1, kMiddle = 2 };
enum class InputAction { kPress = 1, kRelease = 0, kRepeat = 2 };
```

回调类型：`InputEventCallback(button, action, pos)`、
`ScrollEventCallback(xoffset, yoffset)`、`ResizeCallback(w, h)`、
`MouseMoveCallback(pos)`。
