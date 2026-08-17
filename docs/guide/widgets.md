# Widget System

NeoFlux uses a Flutter-style widget system. UIs are built by composing widget
objects into a tree. Each widget overrides virtual functions to customize its
behavior.

## Widget Base Class

`Widget` is the abstract base class for all UI elements. Key virtual functions:

| Function | Purpose |
|----------|---------|
| `Build(BuildContext&)` | Returns child widget(s) for stateful widgets |
| `OnMeasure(width, mode, height, mode)` | Returns intrinsic size for leaf widgets |
| `Paint(RenderContext&)` | Generates render commands |
| `HitTest(Point)` | Tests if a point hits this widget |
| `OnPointerDown(const Point&)` | Handles press event (returns true if consumed) |
| `OnPointerUp(const Point&)` | Handles release event |
| `OnPointerMove(const Point&)` | Handles pointer move (hover/drag) |
| `OnPointerEnter()` | Handles pointer entering widget bounds |
| `OnPointerExit()` | Handles pointer leaving widget bounds |
| `OnPointerScroll(const Point&, x, y)` | Handles scroll events |
| `GetWidgetName()` | Returns the widget's name for debugging (string_view) |

## Widget Lifecycle

```
Create → Build → Layout → Paint → Display
                ↑                    │
                └──── dirty flag ────┘
```

1. **Create**: Widget is constructed and added to the tree.
2. **Build**: `Build()` is called for dirty widgets to create/update children.
3. **Layout**: Taitank computes positions and sizes.
4. **Paint**: `Paint()` generates render commands.
5. **Display**: Render layer draws the frame.

When a widget calls `MarkNeedsBuild()`, it is rebuilt on the next frame. When
the application calls `MarkFrameDirty()`, a full layout/paint cycle is triggered.

## Container

`Container` is the primary layout widget. It maps directly to a Taitank flex
node and supports:

- Flex direction (row/column)
- Justify content (start/center/end/space-between/space-around)
- Align items (start/center/end/stretch)
- Padding and margin
- Background color
- Border radius
- Flex grow/shrink

```cpp
auto col = std::make_shared<Container>();
col->SetFlexDirection(FlexDirection::kColumn)
   .SetJustifyContent(HAlign::kCenter)
   .SetAlignItems(VAlign::kCenter)
   .SetPadding({.left = 16, .top = 16, .right = 16, .bottom = 16})
   .SetBackgroundColor({.r = 255, .g = 255, .b = 255, .a = 255})
   .SetBorderRadius(8.0F);
```

## Text

`Text` renders a single-line string. It is a leaf widget that reports its
intrinsic size via `OnMeasure()`.

```cpp
auto label = std::make_shared<Text>("Hello World");
label->SetFontSize(18.0F)
     .SetTextColor({.r = 0, .g = 0, .b = 0, .a = 255})
     .SetAlignment(HAlign::kCenter)
     .SetFont("NotoSansSC-Regular");  // optional: font by name
```

## Button

`Button` is a clickable widget with a label and press callback.

```cpp
auto btn = std::make_shared<Button>("Click Me");
btn->SetOnPressed([]() {
    LOG(INFO) << "Button pressed!";
});
btn->SetFontSize(16.0F)
   .SetBackgroundColor({.r = 33, .g = 150, .b = 243, .a = 255})
   .SetTextColor({.r = 255, .g = 255, .b = 255, .a = 255})
   .SetBorderRadius(6.0F);
```

## ScrollView

`ScrollView` provides a scrollable viewport. Content larger than the viewport
can be panned via mouse wheel (desktop) or touch drag (mobile).

```cpp
auto scroll = std::make_shared<ScrollView>();

auto content = std::make_shared<Container>();
content->SetFlexDirection(FlexDirection::kColumn);
for (int i = 0; i < 20; ++i) {
  auto item = std::make_shared<Text>("Item " + std::to_string(i));
  content->AddChild(item);
}

scroll->SetContent(content);
parent->AddChild(scroll);
```

## Expanded

`Expanded` is a convenience widget that sets `flex_grow` to fill remaining
space in a flex parent.

```cpp
auto row = std::make_shared<Container>();
row->SetFlexDirection(FlexDirection::kRow);

auto left = std::make_shared<Expanded>(std::make_shared<Text>("Left"));
auto right = std::make_shared<Expanded>(std::make_shared<Text>("Right"), 2);
row->AddChild(left);
row->AddChild(right);  // takes 2x the space of left
```

## SizedBox

`SizedBox` is a container with explicit width and height. Useful for fixed-size
spacing or dimensions.

```cpp
auto spacer = std::make_shared<SizedBox>(0, 16);  // 16px vertical spacer
parent->AddChild(spacer);

auto card = std::make_shared<SizedBox>(200, 120);
card->SetBackgroundColor({.r = 240, .g = 240, .b = 240, .a = 255});
```

## StatefulWidget

`StatefulWidget` holds mutable state that can trigger rebuilds. Override
`Build()` to return the widget tree based on current state.

```cpp
class CounterWidget : public StatefulWidget {
 public:
  std::shared_ptr<Widget> Build(BuildContext& context) override {
    auto col = std::make_shared<Container>();
    auto label = std::make_shared<Text>("Count: " + std::to_string(count_));
    auto btn = std::make_shared<Button>("Increment");
    btn->SetOnPressed([this]() {
      count_++;
      MarkNeedsBuild();
    });
    col->AddChild(label);
    col->AddChild(btn);
    return col;
  }
 private:
  int count_ = 0;
};
```

Call `MarkNeedsBuild()` when state changes to schedule a rebuild.
