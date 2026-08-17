# Container

Flexbox layout widget that maps to a Taitank node. The primary building block
for layouts.

## Header

```cpp
#include <neoflux/widget/container.h>
```

## Construction

```cpp
auto container = std::make_shared<Container>();
```

## Layout Methods

All methods return `Container&` for chaining.

### `SetFlexDirection()`

```cpp
Container& SetFlexDirection(FlexDirection direction);
```

Sets the main axis direction. `FlexDirection::kRow` or `FlexDirection::kColumn`.

### `SetJustifyContent()`

```cpp
Container& SetJustifyContent(HAlign align);
```

Sets alignment along the main axis. Values: `kStart`, `kCenter`, `kEnd`,
`kSpaceBetween`, `kSpaceAround`.

### `SetAlignItems()`

```cpp
Container& SetAlignItems(VAlign align);
```

Sets alignment along the cross axis. Values: `kStart`, `kCenter`, `kEnd`,
`kStretch`.

### `SetFlexGrow()`

```cpp
Container& SetFlexGrow(float grow);
```

Sets the flex grow factor. Determines how much the item grows relative to
siblings.

### `SetFlexShrink()`

```cpp
Container& SetFlexShrink(float shrink);
```

Sets the flex shrink factor.

### `SetWidth()` / `SetHeight()`

```cpp
Container& SetWidth(float width);
Container& SetHeight(float height);
```

Sets explicit dimensions.

## Spacing

### `SetPadding()`

```cpp
Container& SetPadding(const EdgeInsets& padding);
```

Sets padding. `EdgeInsets` has fields: `left`, `top`, `right`, `bottom`.

### `SetMargin()`

```cpp
Container& SetMargin(const EdgeInsets& margin);
```

Sets margin.

## Appearance

### `SetBackgroundColor()`

```cpp
Container& SetBackgroundColor(const Color& color);
```

Sets the background color. `Color` has fields: `r`, `g`, `b`, `a` (0-255).

### `SetBorderRadius()`

```cpp
Container& SetBorderRadius(float radius);
```

Sets the corner radius in pixels. 0 = sharp corners.

## Example

```cpp
auto card = std::make_shared<Container>();
card->SetFlexDirection(FlexDirection::kColumn)
    .SetPadding({.left = 16, .top = 12, .right = 16, .bottom = 12})
    .SetMargin({.bottom = 8})
    .SetBackgroundColor({.r = 255, .g = 255, .b = 255, .a = 255})
    .SetBorderRadius(8.0F);

card->AddChild(title);
card->AddChild(body);
```
