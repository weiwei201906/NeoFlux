# Flex Layout

A layout showcase demonstrating Taitank flex layout capabilities.

## Run

```bash
./bin/flex_demo
```

## Features

- Row and column flex directions
- Justify content (start, center, end)
- Align items (start, center, end)
- Flex grow (proportional sizing)
- Row reverse
- Padding and margin
- Nested containers

## Layout Sections

1. **Row - Center**: Three colored boxes centered horizontally.
2. **Column - Center**: Three colored boxes centered vertically.
3. **Row - Flex Grow**: Two boxes with flex 1 and flex 2.
4. **Row Reverse**: Boxes in reverse order.

## Key Code

```cpp
auto row = std::make_shared<Container>();
row->SetFlexDirection(FlexDirection::kRow)
   .SetJustifyContent(HAlign::kCenter)
   .SetAlignItems(VAlign::kCenter)
   .SetHeight(60.0F);

auto box_a = MakeBox("A", {.r = 244, .g = 67, .b = 54, .a = 255}, 50, 50);
auto box_b = MakeBox("B", {.r = 76, .g = 175, .b = 80, .a = 255}, 50, 50);
auto box_c = MakeBox("C", {.r = 33, .g = 150, .b = 243, .a = 255}, 50, 50);

row->AddChild(box_a);
row->AddChild(box_b);
row->AddChild(box_c);
```

## Resize

Try resizing the window. The layout adapts automatically as Taitank
recomputes positions and sizes.
