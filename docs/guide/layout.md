# Flex Layout

NeoFlux uses [Taitank](https://github.com/Tencent/taitank), a high-performance
flexbox layout engine from Tencent, for all widget positioning and sizing.

## How It Works

Every `Container` creates a Taitank flex node. When `PerformLayout()` is called
on the root widget, Taitank recursively computes the position and size of every
node in the tree. The results are copied back into each widget's `bounds_` via
`ReadLayoutRecursive()`.

```cpp
root->PerformLayout(window_width, window_height);
// After layout, each widget's GetBounds() returns its computed position/size.
```

## Flex Direction

Controls the main axis direction:

| Value | Description |
|-------|-------------|
| `FlexDirection::kRow` | Children arranged horizontally (left to right) |
| `FlexDirection::kColumn` | Children arranged vertically (top to bottom) |

```cpp
container->SetFlexDirection(FlexDirection::kRow);
```

::: tip Default
NeoFlux sets the default flex direction to `kColumn` (unlike CSS/Taitank's
default of `kRow`), which is more natural for mobile UIs.
:::

## Justify Content

Controls alignment along the main axis:

| Value | Description |
|-------|-------------|
| `HAlign::kStart` | Pack at start |
| `HAlign::kCenter` | Pack at center |
| `HAlign::kEnd` | Pack at end |
| `HAlign::kSpaceBetween` | Space between items |
| `HAlign::kSpaceAround` | Space around items |

```cpp
container->SetJustifyContent(HAlign::kCenter);
```

## Align Items

Controls alignment along the cross axis:

| Value | Description |
|-------|-------------|
| `VAlign::kStart` | Align to start |
| `VAlign::kCenter` | Align to center |
| `VAlign::kEnd` | Align to end |
| `VAlign::kStretch` | Stretch to fill |

```cpp
container->SetAlignItems(VAlign::kCenter);
```

## Flex Grow

Controls how a child grows to fill available space along the main axis.

```cpp
auto item = std::make_shared<Container>();
item->SetFlexGrow(1.0F);  // takes 1 share of available space
```

Use `Expanded` as a convenience:

```cpp
auto item = std::make_shared<Expanded>(child);  // flex_grow = 1
auto item2 = std::make_shared<Expanded>(child, 2);  // flex_grow = 2
```

## Padding and Margin

```cpp
container->SetPadding({.left = 16, .top = 8, .right = 16, .bottom = 8});
container->SetMargin({.top = 12});
```

## Fixed Sizes

Leaf widgets (`Text`, `Button`) report their intrinsic size automatically.
For containers, you can set explicit dimensions:

```cpp
container->SetWidth(200.0F);
container->SetHeight(100.0F);
```

Or use `SizedBox`:

```cpp
auto box = std::make_shared<SizedBox>(200, 100);
```

## Root Widget

The root widget (no parent) is automatically sized to fill the entire window:

```cpp
// In Widget::PerformLayout:
if (parent_ == nullptr) {
  taitank::SetWidth(node, width);
  taitank::SetHeight(node, height);
}
```

This ensures the root always fills the viewport, even when the window is
resized.

## Common Layout Patterns

### Centered Content

```cpp
auto root = std::make_shared<Container>();
root->SetFlexDirection(FlexDirection::kColumn)
    .SetJustifyContent(HAlign::kCenter)
    .SetAlignItems(VAlign::kCenter);
```

### Header + Scrollable Content

```cpp
auto root = std::make_shared<Container>();
root->SetFlexDirection(FlexDirection::kColumn);

auto header = std::make_shared<Container>();
header->SetHeight(56.0F);
root->AddChild(header);

auto scroll = std::make_shared<ScrollView>();
scroll->SetContent(big_content);
root->AddChild(scroll);  // fills remaining space (flex_grow=1)
```

### Two-Column Layout

```cpp
auto row = std::make_shared<Container>();
row->SetFlexDirection(FlexDirection::kRow);

auto left = std::make_shared<Expanded>(left_content);
auto right = std::make_shared<Expanded>(right_content, 2);  // 2x wider
row->AddChild(left);
row->AddChild(right);
```
