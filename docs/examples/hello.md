# Hello NeoFlux

A complete demo showing stateful widgets, button callbacks, route navigation,
and flex layout.

## Run

```bash
./bin/hello_neoflux
```

## Features

- Stateful widget with counter
- Button press callbacks
- Route navigation (push/pop)
- Flex layout (column, center alignment)
- Background colors and padding

## Key Code

```cpp
class HomePage : public StatefulWidget {
 public:
  std::shared_ptr<Widget> Build(BuildContext&) override {
    auto root = std::make_shared<Container>();
    root->SetFlexDirection(FlexDirection::kColumn)
        .SetJustifyContent(HAlign::kCenter)
        .SetAlignItems(VAlign::kCenter)
        .SetBackgroundColor({.r = 245, .g = 245, .b = 245, .a = 255});

    auto title = std::make_shared<Text>("Hello NeoFlux");
    title->SetFontSize(28.0F);
    root->AddChild(title);

    auto count_label = std::make_shared<Text>("Count: " + std::to_string(count_));
    root->AddChild(count_label);

    auto btn = std::make_shared<Button>("Increment");
    btn->SetOnPressed([this]() { count_++; MarkNeedsBuild(); });
    root->AddChild(btn);

    return root;
  }
 private:
  int count_ = 0;
};
```
