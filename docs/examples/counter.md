# Counter

A minimal counter app demonstrating `StatefulWidget` and `Button` callbacks.

## Run

```bash
./bin/counter
```

## Features

- Increment/decrement buttons
- Stateful counter display
- Row layout with buttons

## Key Code

```cpp
class CounterPage : public StatefulWidget {
 public:
  std::shared_ptr<Widget> Build(BuildContext&) override {
    auto root = std::make_shared<Container>();
    root->SetFlexDirection(FlexDirection::kColumn)
        .SetJustifyContent(HAlign::kCenter)
        .SetAlignItems(VAlign::kCenter);

    auto label = std::make_shared<Text>("Count: " + std::to_string(count_));
    label->SetFontSize(32.0F);
    root->AddChild(label);

    auto row = std::make_shared<Container>();
    row->SetFlexDirection(FlexDirection::kRow)
       .SetJustifyContent(HAlign::kCenter)
       .SetMargin({.top = 16});

    auto dec = std::make_shared<Button>("-");
    dec->SetOnPressed([this]() { count_--; MarkNeedsBuild(); });
    auto inc = std::make_shared<Button>("+");
    inc->SetOnPressed([this]() { count_++; MarkNeedsBuild(); });

    row->AddChild(dec);
    row->AddChild(inc);
    root->AddChild(row);
    return root;
  }
 private:
  int count_ = 0;
};
```
