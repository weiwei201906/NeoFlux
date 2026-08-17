# SizedBox

A `Container` with explicit width and height. Useful for fixed-size elements
and spacing.

## Header

```cpp
#include <neoflux/widget/sized_box.h>
```

## Construction

```cpp
// Empty box (spacer)
auto spacer = std::make_shared<SizedBox>();

// Fixed size
auto box = std::make_shared<SizedBox>(200.0F, 100.0F);
```

## Methods

### `SetWidth()`

```cpp
SizedBox& SetWidth(float width);
```

Sets the width. Only applies if `width > 0`.

### `SetHeight()`

```cpp
SizedBox& SetHeight(float height);
```

Sets the height. Only applies if `height > 0`.

### `SetSize()`

```cpp
SizedBox& SetSize(float width, float height);
```

Sets both dimensions.

## Example

```cpp
// Vertical spacer of 16px
auto spacer = std::make_shared<SizedBox>(0, 16);
column->AddChild(spacer);

// Fixed-size card
auto card = std::make_shared<SizedBox>(200, 120);
card->SetBackgroundColor({.r = 240, .g = 240, .b = 240, .a = 255})
    .SetBorderRadius(8.0F);
card->AddChild(content);
```

## Notes

- `SizedBox` inherits all `Container` methods.
- A dimension of 0 or negative means "auto" (Taitank decides the size).
- Use `SizedBox(0, 0)` or `SizedBox()` for an auto-sized container.
