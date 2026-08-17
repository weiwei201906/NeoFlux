# Expanded

A convenience `Container` with `flex_grow` set. Fills remaining space in a
flex parent.

## Header

```cpp
#include <neoflux/widget/expanded.h>
```

## Construction

```cpp
// flex_grow = 1 (default)
auto expanded = std::make_shared<Expanded>(child);

// flex_grow = 2
auto expanded2 = std::make_shared<Expanded>(child, 2);
```

## Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `child` | `std::shared_ptr<Widget>` | required | The child widget |
| `flex` | `int` | `1` | The flex grow factor |

## Example

```cpp
auto row = std::make_shared<Container>();
row->SetFlexDirection(FlexDirection::kRow);

auto left_panel = std::make_shared<Expanded>(left_content);      // 1 share
auto right_panel = std::make_shared<Expanded>(right_content, 2); // 2 shares

row->AddChild(left_panel);
row->AddChild(right_panel);
```

In this example, `right_panel` takes twice the space of `left_panel`.

## Notes

- `Expanded` inherits all `Container` methods (padding, margin, background,
  etc.).
- Only meaningful as a child of a flex container (`Container` with row or
  column direction).
- Equivalent to `Container::SetFlexGrow(flex)`.
