# Fluent API / 链式写法

NeoFlux 支持两种等价的 widget 树构建方式：显式的 `std::make_shared` +
`AddChild` 写法，以及 **流式构建（fluent builder）** 写法——setter 与子节点添加方法都返回
widget 引用，可以在一条表达式里链式搭完整棵树。

::: tip 真实方法名
setter 是 **大驼峰（PascalCase）**（`SetBackgroundColor`、`SetText` 等）。流式子节点方法是
小写（`child`、`children`），链尾终止符是小写 `build()`。不存在 snake_case 别名——请严格使用
头文件里的名字。
:::

## 为什么有两种写法？

- **显式写法** 自上而下阅读，调试时容易单步，子节点由数据动态生成时最顺手。
- **流式写法** 把一棵静态布局描述成一条嵌套表达式，读起来像布局本身，也无需为每个中间节点命名。

两种写法产出的 `shared_ptr<Widget>` 树完全相同。

## 链是如何工作的

三个要素：

1. 每个 setter 都 **按引用返回具体的 widget 类型**——例如
   `Container& SetBackgroundColor(const Color&)`、`Text& SetFontSize(float)`。
   因此 `widget->SetA(x).SetB(y)` 可用。
2. `Container::child(std::shared_ptr<Widget>)` 与
   `Container::children(std::initializer_list<std::shared_ptr<Widget>>)` 追加子节点并返回
   `Container&`。
3. `Widget::build()`（小写）是链尾终止符，返回 `std::shared_ptr<Widget>`——即框架所需的持有句柄。

因为种子是一个临时 `shared_ptr<Container>`，第一个子节点调用用 `->`，之后都用 `.`
作用在返回的引用上：

```cpp
auto view = std::make_shared<Container>()
    ->child(std::make_shared<Text>("Hi"))    // 种子：对临时 shared_ptr 用 ->
    .child(std::make_shared<Button>("Go"))   // 之后对 Container& 用 .
    .build();                                 // 终止符 -> shared_ptr<Widget>
```

::: warning 仅在由 shared_ptr 管理的 widget 上调用 build()
`build()` 内部使用 `shared_from_this()`，只有在 `std::make_shared` 之后才合法。
切勿在栈上的 widget 上调用它。
:::

## 显式 vs 流式：同一个界面

### 显式写法

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

### 流式写法

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

注意：

- 外层容器以 `->SetBackgroundColor(...)` 开始（对 `shared_ptr` 取成员），随后用 `.`
  链式调用 `.SetPadding` / `.child`。
- 嵌套叶子（`Text`、`Button`）本身也是一条子链，由各自的
  `std::make_shared<Text>(...)` 起手，以 `.build()` 结束。

## 一次性添加多个子节点

`children()` 接受初始化列表，读起来就像一棵字面量子树：

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

## 可链式方法速查

下列每个 setter 都返回其具体 widget 引用（可继续链式调用）。

| 类 | 可链式方法（返回 `X&`） |
|----|------------------------|
| `Container` | `SetBackgroundColor`、`SetPadding`、`SetMargin`、`SetWidth`、`SetHeight`、`SetChild`、`child`、`children`、`SetFlexDirection`、`SetJustifyContent`、`SetAlignItems`、`SetFlexGrow`、`SetBorderRadius` |
| `Text` | `SetText`、`SetTextColor`、`SetFontSize`、`SetAlignment`、`SetFont` |
| `Button` | `SetLabel`、`SetOnPressed`、`SetBackgroundColor`、`SetTextColor`、`SetFontSize`、`SetFont` |
| `SizedBox` | `SetWidth`、`SetHeight`、`SetSize` |
| `Expanded` | `SetFlex` |
| `Divider` | `SetColor`、`SetThickness`、`SetOrientation` |
| `Align` | `SetHorizontal`、`SetVertical` |
| `Card` | `SetCardColor`、`SetCardRadius`、`SetCardPadding` |
| `ProgressIndicator` | `SetValue`、`SetTrackColor`、`SetFillColor`、`SetThickness` |
| `Switch` | `SetChecked`、`SetOnChanged`、`SetLabel`、`SetOnColor`、`SetOffColor` |
| `Checkbox` | `SetChecked`、`SetOnChanged`、`SetLabel`、`SetCheckedColor` |
| `MediaWidget` | `SetSource`、`SetVolume`、`SetBackgroundColor`、`SetTextColor` |

## 建议

静态布局（一次性描述的界面）用流式链；子节点由循环或运行时数据生成时，回到显式的
`make_shared` + `AddChild`。两种风格可在同一棵树里自由混用。
