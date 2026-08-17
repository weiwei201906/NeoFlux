# StatefulWidget

Base class for widgets that hold mutable state. Override `Build()` to return
the widget tree based on current state.

## Header

```cpp
#include <neoflux/widget/widget.h>
```

## Overview

`StatefulWidget` extends `Widget` with state management. When state changes,
call `MarkNeedsBuild()` to schedule a rebuild.

## Example

```cpp
class CounterWidget : public StatefulWidget {
 public:
  std::string_view GetWidgetName() const noexcept override {
    return "CounterWidget";
  }

  std::shared_ptr<Widget> Build(BuildContext& /*context*/) override {
    auto col = std::make_shared<Container>();
    col->SetFlexDirection(FlexDirection::kColumn)
       .SetJustifyContent(HAlign::kCenter)
       .SetAlignItems(VAlign::kCenter);

    auto label = std::make_shared<Text>("Count: " + std::to_string(count_));
    label->SetFontSize(24.0F);
    col->AddChild(label);

    auto btn = std::make_shared<Button>("Increment");
    btn->SetOnPressed([this]() {
      count_++;
      MarkNeedsBuild();
    });
    col->AddChild(btn);

    return col;
  }

 private:
  int count_ = 0;
};
```

## Key Points

- `Build()` is called when the widget is dirty (`MarkNeedsBuild()` was called).
- After `Build()`, the old children are replaced with the new widget tree.
- State is preserved across rebuilds (the widget object itself is not recreated).
- Call `MarkNeedsBuild()` whenever state changes to trigger a rebuild.

## Lifecycle

```
Create → Build (initial) → Layout → Paint
              ↑                      │
              └── MarkNeedsBuild() ──┘
```
