# Container

```cpp
class Container : public Widget;
```

The primary layout widget. It maps 1:1 to a Taitank flexbox node: children are
laid out by Taitank, and the Container only paints its own rounded background.

## Header

```cpp
#include <neoflux/widgets/container.h>
```

## Construction

```cpp
auto container = std::make_shared<Container>();
```

## Adding children

| Method | Signature | Notes |
|--------|-----------|-------|
| `SetChild` | `Container& SetChild(std::shared_ptr<Widget> child)` | Single-child convenience (chainable, returns `Container&`) |
| `child` | `Container& child(std::shared_ptr<Widget> child)` | Append one child (chainable, returns `Container&`) |
| `children` | `Container& children(std::initializer_list<std::shared_ptr<Widget>> children)` | Append many (chainable, returns `Container&`) |
| `build` | `[[nodiscard]] std::shared_ptr<Widget> build()` | Inherited from `Widget`; chain terminator |

See the [Fluent API](../guide/fluent-api.md) guide for the chained idiom.

## Layout knobs (all chainable, return `Container&`)

| Method | Signature | Meaning |
|--------|-----------|---------|
| `SetFlexDirection` | `Container& SetFlexDirection(FlexDirection d)` | Main axis. `kRow` / `kRowReverse` / `kColumn` / `kColumnReverse`. Default `kColumn`. |
| `SetJustifyContent` | `Container& SetJustifyContent(HAlign a)` | Main-axis alignment: `HAlign::kLeft/kCenter/kRight`. |
| `SetAlignItems` | `Container& SetAlignItems(VAlign a)` | Cross-axis alignment: `VAlign::kTop/kCenter/kBottom`. |
| `SetFlexGrow` | `Container& SetFlexGrow(float grow)` | How much this container expands to fill parent space. |
| `SetWidth` | `Container& SetWidth(float w)` | Fixed width; 0 = flexible. |
| `SetHeight` | `Container& SetHeight(float h)` | Fixed height; 0 = flexible. |

## Spacing & appearance (chainable, return `Container&`)

| Method | Signature | Meaning |
|--------|-----------|---------|
| `SetPadding` | `Container& SetPadding(const EdgeInsets& p)` | Inner padding. |
| `SetMargin` | `Container& SetMargin(const EdgeInsets& m)` | Outer margin. |
| `SetBackgroundColor` | `Container& SetBackgroundColor(const Color& c)` | Background fill. |
| `SetBorderRadius` | `Container& SetBorderRadius(float r)` | Corner radius in px; 0 = sharp. |

## Example

```cpp
auto card = std::make_shared<Container>();
card->SetFlexDirection(FlexDirection::kColumn)
    .SetPadding({.left = 16, .top = 12, .right = 16, .bottom = 12})
    .SetBackgroundColor({.r = 255, .g = 255, .b = 255, .a = 255})
    .SetBorderRadius(8.0F);
card->AddChild(title);
card->AddChild(body);
```

Equivalent fluent form:

```cpp
auto card = std::make_shared<Container>()
    ->SetFlexDirection(FlexDirection::kColumn)
    .SetPadding({.left = 16, .top = 12, .right = 16, .bottom = 12})
    .SetBackgroundColor({255, 255, 255, 255})
    .SetBorderRadius(8.0F)
    .children({title, body})
    .build();
```
