# Fluent API / Chained construction

NeoFlux supports two equivalent styles for building a widget tree. You can use
the explicit `std::make_shared` + `AddChild` style, or the **fluent builder**
style where setters and child-adders return the widget reference so you can
chain everything in one expression.

::: tip Real method names
Setters are **PascalCase** (`SetBackgroundColor`, `SetText`, ...). The fluent
child-adders are lowercase (`child`, `children`), and the chain terminator is
lowercase `build()`. There are no snake_case aliases — use exactly the names in
the headers.
:::

## Why two styles?

- **Explicit style** reads top-to-bottom, is easy to step through in a debugger,
  and is what you want when children are computed dynamically.
- **Fluent style** describes a static tree as one nested expression, which reads
  like a layout and avoids naming every intermediate node.

Both produce the exact same `shared_ptr<Widget>` tree.

## How the chain works

Three pieces make it possible:

1. Every setter returns the **concrete widget type by reference** — e.g.
   `Container& SetBackgroundColor(const Color&)`, `Text& SetFontSize(float)`.
   So `widget->SetA(x).SetB(y)` works.
2. `Container::child(std::shared_ptr<Widget>)` and
   `Container::children(std::initializer_list<std::shared_ptr<Widget>>)` append
   children and return `Container&`.
3. `Widget::build()` (lowercase) is the chain terminator. It returns
   `std::shared_ptr<Widget>` — the owning handle the framework expects.

Because the seed is a `shared_ptr<Container>` temporary, the first child call
uses `->`; everything after that is `.` on the returned reference:

```cpp
auto view = std::make_shared<Container>()
    ->child(std::make_shared<Text>("Hi"))    // seed: -> off the temporary shared_ptr
    .child(std::make_shared<Button>("Go"))  // then . on the Container&
    .build();                                // terminator -> shared_ptr<Widget>
```

::: warning Only call build() on a shared_ptr-managed widget
`build()` uses `shared_from_this()`. It is valid only after `std::make_shared`.
Never call it on a stack-allocated widget.
:::

## Explicit vs. fluent: the same screen

### Explicit style

```cpp
std::shared_ptr<Widget> BuildHome(BuildContext&) {
  auto root = std::make_shared<Container>();
  root->SetBackgroundColor({255, 255, 255, 255});
  root->SetPadding({.left = 24, .top = 24, .right = 24, .bottom = 24});
  root->SetFlexDirection(FlexDirection::kColumn);

  auto title = std::make_shared<Text>("Hello");
  title->SetFontSize(28.0F).SetTextColor({10, 10, 10, 255});
  root->AddChild(title);

  auto button = std::make_shared<Button>("OK");
  button->SetOnPressed([] { /* ... */ });
  root->AddChild(button);

  return root;
}
```

### Fluent style

```cpp
std::shared_ptr<Widget> BuildHome(BuildContext&) {
  return std::make_shared<Container>()
      ->SetBackgroundColor({255, 255, 255, 255})
      .SetPadding({.left = 24, .top = 24, .right = 24, .bottom = 24})
      .SetFlexDirection(FlexDirection::kColumn)
      .child(std::make_shared<Text>("Hello")
                 ->SetFontSize(28.0F)
                 .SetTextColor({10, 10, 10, 255})
                 .build())
      .child(std::make_shared<Button>("OK")
                 ->SetOnPressed([] { /* ... */ })
                 .build())
      .build();
}
```

Notice:

- The outer container seeds with `->SetBackgroundColor(...)` (off the
  `shared_ptr`), then chains `.SetPadding` / `.child` with `.`.
- A nested leaf (`Text`, `Button`) is itself built as a sub-chain seeded by its
  own `std::make_shared<Text>(...)`, ending in `.build()`.

## Adding many children at once

`children()` takes an initializer list, which reads like a literal subtree:

```cpp
return std::make_shared<Container>()
    ->SetFlexDirection(FlexDirection::kColumn)
    .children({
        std::make_shared<Text>("Row one")->build(),
        std::make_shared<Divider>()->build(),
        std::make_shared<Text>("Row two")->build(),
    })
    .build();
```

## Chainable reference

Every setter below returns its concrete widget reference (so calls chain).
The symbol "↩ returns" marks fluent methods.

| Class | Chainable methods (return `X&`) |
|-------|--------------------------------|
| `Container` | `SetBackgroundColor`, `SetPadding`, `SetMargin`, `SetWidth`, `SetHeight`, `SetChild`, `child`, `children`, `SetFlexDirection`, `SetJustifyContent`, `SetAlignItems`, `SetFlexGrow`, `SetBorderRadius` |
| `Text` | `SetText`, `SetTextColor`, `SetFontSize`, `SetAlignment`, `SetFont` |
| `Button` | `SetLabel`, `SetOnPressed`, `SetBackgroundColor`, `SetTextColor`, `SetFontSize`, `SetFont` |
| `SizedBox` | `SetWidth`, `SetHeight`, `SetSize` (inherited container setters also return `SizedBox&` where overridden) |
| `Expanded` | `SetFlex` |
| `Divider` | `SetColor`, `SetThickness`, `SetOrientation` |
| `Align` | `SetHorizontal`, `SetVertical` |
| `Card` | `SetCardColor`, `SetCardRadius`, `SetCardPadding` |
| `ProgressIndicator` | `SetValue`, `SetTrackColor`, `SetFillColor`, `SetThickness` |
| `Switch` | `SetChecked`, `SetOnChanged`, `SetLabel`, `SetOnColor`, `SetOffColor` |
| `Checkbox` | `SetChecked`, `SetOnChanged`, `SetLabel`, `SetCheckedColor` |
| `MediaWidget` | `SetSource`, `SetVolume`, `SetBackgroundColor`, `SetTextColor` |

## Recommendation

Use fluent chaining for **static layouts** (screens you describe once). Drop to
explicit `make_shared` + `AddChild` when children are produced by a loop or
depend on runtime data. You can mix both freely in the same tree.
