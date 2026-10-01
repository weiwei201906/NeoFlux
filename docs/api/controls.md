# Layout & Control Widgets

This page covers the smaller, purpose-built widgets. Each setter that returns the
concrete widget reference is marked **(chainable, returns `X&`)**.

---

## Divider

```cpp
class Divider : public Container;
enum class DividerOrientation { kHorizontal, kVertical };
```

A thin separator line. Header: `<neoflux/widgets/divider.h>`.

| Method | Signature | Notes |
|--------|-----------|-------|
| ctor | `Divider()` / `explicit Divider(float thickness)` | |
| `SetColor` | `Divider& SetColor(const Color&)` | **(chainable)** |
| `SetThickness` | `Divider& SetThickness(float)` | px, default 1.0. **(chainable)** |
| `SetOrientation` | `Divider& SetOrientation(DividerOrientation)` | default horizontal. **(chainable)** |

```cpp
std::make_shared<Divider>(1.0F)
    ->SetColor({220, 220, 220, 255})
    .build();
```

---

## Padding

```cpp
class Padding : public Container;
```

A single-child widget that insets its child by fixed edge insets. Header:
`<neoflux/widgets/padding.h>`. No setters; child + insets are supplied up front.

| Method | Signature |
|--------|-----------|
| ctor | `explicit Padding(float all, std::shared_ptr<Widget> child = nullptr)` |
| ctor | `Padding(const EdgeInsets& insets, std::shared_ptr<Widget> child = nullptr)` |

```cpp
std::make_shared<Padding>(16.0F, label);  // uniform 16px all edges
```

---

## Center

```cpp
class Center : public Container;
```

Fills its parent and centers the child on both axes (flex-grow=1, centered
justify/align). Header: `<neoflux/widgets/center.h>`.

| Method | Signature |
|--------|-----------|
| ctor | `explicit Center(std::shared_ptr<Widget> child = nullptr)` |

```cpp
std::make_shared<Center>(spinner);
```

---

## Align

```cpp
class Align : public Container;
```

Like `Center` but for an arbitrary corner/alignment. Equivalent to
`Align(kCenter, kCenter)` being `Center`. Header: `<neoflux/widgets/align.h>`.

| Method | Signature | Notes |
|--------|-----------|-------|
| ctor | `Align(HAlign h, VAlign v, std::shared_ptr<Widget> child = nullptr)` | |
| `SetHorizontal` | `Align& SetHorizontal(HAlign)` | **(chainable)** |
| `SetVertical` | `Align& SetVertical(VAlign)` | **(chainable)** |

```cpp
std::make_shared<Align>(HAlign::kRight, VAlign::kTop, closeButton);
```

---

## Card

```cpp
class Card : public Container;
```

A rounded, padded surface panel framing a single child. Adapted (design-level)
from EUI-NEO, Apache-2.0. Header: `<neoflux/widgets/card.h>`.

| Method | Signature | Notes |
|--------|-----------|-------|
| ctor | `explicit Card(std::shared_ptr<Widget> child = nullptr)` | |
| `SetCardColor` | `Card& SetCardColor(const Color&)` | **(chainable)** |
| `SetCardRadius` | `Card& SetCardRadius(float)` | px. **(chainable)** |
| `SetCardPadding` | `Card& SetCardPadding(float)` | uniform inner padding. **(chainable)** |

```cpp
std::make_shared<Card>(body)
    ->SetCardColor({255, 255, 255, 255})
    .SetCardRadius(10.0F)
    .SetCardPadding(14.0F)
    .build();
```

---

## ProgressIndicator

```cpp
class ProgressIndicator : public Widget;
```

A determinate horizontal progress bar (rounded track + filled portion). Adapted
from EUI-NEO, Apache-2.0. Header: `<neoflux/widgets/progress_indicator.h>`.

| Method | Signature | Notes |
|--------|-----------|-------|
| ctor | `ProgressIndicator()` | |
| `SetValue` | `ProgressIndicator& SetValue(float)` | clamped to [0,1]. **(chainable)** |
| `GetValue` | `float GetValue() const noexcept` | current clamped value. |
| `SetTrackColor` | `ProgressIndicator& SetTrackColor(const Color&)` | background. **(chainable)** |
| `SetFillColor` | `ProgressIndicator& SetFillColor(const Color&)` | filled portion. **(chainable)** |
| `SetThickness` | `ProgressIndicator& SetThickness(float)` | track height px. **(chainable)** |

```cpp
bar->SetValue(0.6F).SetFillColor({40, 140, 255, 255});
```

---

## Switch

```cpp
class Switch : public Widget;
using OnChanged = std::function<void(bool checked)>;
```

A pill toggle. Clicking toggles state and fires the callback. Adapted from
EUI-NEO, Apache-2.0. Header: `<neoflux/widgets/switch.h>`.

| Method | Signature | Notes |
|--------|-----------|-------|
| ctor | `Switch()` / `explicit Switch(bool checked)` | |
| `SetChecked` | `Switch& SetChecked(bool)` | programmatic; does NOT fire callback. **(chainable)** |
| `IsChecked` | `bool IsChecked() const noexcept` | |
| `SetOnChanged` | `Switch& SetOnChanged(OnChanged)` | fired on user toggle. **(chainable)** |
| `SetLabel` | `Switch& SetLabel(std::string)` | trailing label. **(chainable)** |
| `SetOnColor` | `Switch& SetOnColor(const Color&)` | track when on. **(chainable)** |
| `SetOffColor` | `Switch& SetOffColor(const Color&)` | track when off. **(chainable)** |

```cpp
auto sw = std::make_shared<Switch>(false);
sw->SetLabel("Dark mode")
  .SetOnChanged([](bool on) { /* ... */ });
```

---

## Checkbox

```cpp
class Checkbox : public Widget;
using OnChanged = std::function<void(bool checked)>;
```

A checked/unchecked box with optional trailing label. The check mark is drawn as
a filled block (the command buffer exposes rects only). Adapted from EUI-NEO,
Apache-2.0. Header: `<neoflux/widgets/checkbox.h>`.

| Method | Signature | Notes |
|--------|-----------|-------|
| ctor | `Checkbox()` / `explicit Checkbox(bool checked)` | |
| `SetChecked` | `Checkbox& SetChecked(bool)` | programmatic; does NOT fire callback. **(chainable)** |
| `IsChecked` | `bool IsChecked() const noexcept` | |
| `SetOnChanged` | `Checkbox& SetOnChanged(OnChanged)` | fired on toggle. **(chainable)** |
| `SetLabel` | `Checkbox& SetLabel(std::string)` | trailing label. **(chainable)** |
| `SetCheckedColor` | `Checkbox& SetCheckedColor(const Color&)` | box color when checked. **(chainable)** |

```cpp
auto cb = std::make_shared<Checkbox>();
cb->SetLabel("Remember me")
  .SetOnChanged([](bool on) { /* ... */ });
```
