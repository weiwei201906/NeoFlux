# Scroll View

Demonstrates `ScrollView`: a header bar plus a scrollable list of colored items.

## Run

```bash
./bin/scroll_demo
```

## Features

- Fixed header bar
- Scrollable content area
- Mouse wheel scrolling
- Clip rect (content outside viewport is hidden)
- 14 colored list items exceeding viewport height

## Key Code

```cpp
auto root = std::make_shared<Container>();
root->SetFlexDirection(FlexDirection::kColumn);

// Fixed header
auto header = std::make_shared<Container>();
header->SetHeight(56.0F)
      .SetBackgroundColor({.r = 33, .g = 150, .b = 243, .a = 255})
      .SetJustifyContent(HAlign::kCenter)
      .SetAlignItems(VAlign::kCenter);
header->AddChild(std::make_shared<Text>("Scroll View Demo"));
root->AddChild(header);

// Scrollable content
auto scroll = std::make_shared<ScrollView>();
auto content = std::make_shared<Container>();
content->SetFlexDirection(FlexDirection::kColumn);

for (int i = 0; i < 14; ++i) {
  auto item = std::make_shared<Container>();
  item->SetHeight(60.0F)
      .SetBackgroundColor(colors[i % 14])
      .SetJustifyContent(HAlign::kCenter)
      .SetAlignItems(VAlign::kCenter);
  item->AddChild(std::make_shared<Text>("Item " + std::to_string(i + 1)));
  content->AddChild(item);
}

scroll->SetContent(content);
root->AddChild(scroll);
```

## How It Works

1. `ScrollView` sets `flex_grow = 1` to fill remaining space below the header.
2. Content is laid out at its natural height (14 items × ~68px = ~952px).
3. `ScrollView` clips content to its viewport bounds.
4. Mouse wheel events update the scroll offset and trigger a repaint.
5. Content is translated by `-scroll_offset` during painting.

## Scrolling

Use the mouse wheel to scroll up and down. The scroll offset is clamped to
the valid range (content size minus viewport size).
