# Container

```cpp
class Container : public Widget;
```

最主要的布局组件，与 Taitank flexbox 节点一一对应：子节点由 Taitank 布局，
Container 自身只绘制圆角背景。

## 头文件

```cpp
#include <neoflux/widgets/container.h>
```

## 构造

```cpp
auto container = std::make_shared<Container>();
```

## 添加子节点

| 方法 | 签名 | 说明 |
|------|------|------|
| `SetChild` | `Container& SetChild(std::shared_ptr<Widget> child)` | 单子节点便捷方法（可链式，返回 `Container&`） |
| `child` | `Container& child(std::shared_ptr<Widget> child)` | 追加一个子节点（可链式，返回 `Container&`） |
| `children` | `Container& children(std::initializer_list<std::shared_ptr<Widget>> children)` | 追加多个（可链式，返回 `Container&`） |
| `build` | `[[nodiscard]] std::shared_ptr<Widget> build()` | 继承自 `Widget`，链尾终止符 |

链式写法详见 [流式 API](../guide/fluent-api.md)。

## 布局旋钮（均可链式，返回 `Container&`）

| 方法 | 签名 | 含义 |
|------|------|------|
| `SetFlexDirection` | `Container& SetFlexDirection(FlexDirection d)` | 主轴方向：`kRow` / `kRowReverse` / `kColumn` / `kColumnReverse`，默认 `kColumn`。 |
| `SetJustifyContent` | `Container& SetJustifyContent(HAlign a)` | 主轴对齐：`HAlign::kLeft/kCenter/kRight`。 |
| `SetAlignItems` | `Container& SetAlignItems(VAlign a)` | 交叉轴对齐：`VAlign::kTop/kCenter/kBottom`。 |
| `SetFlexGrow` | `Container& SetFlexGrow(float grow)` | 本容器在父布局中撑满剩余空间的弹性系数。 |
| `SetWidth` | `Container& SetWidth(float w)` | 固定宽度，0 表示自适应。 |
| `SetHeight` | `Container& SetHeight(float h)` | 固定高度，0 表示自适应。 |

## 间距与外观（均可链式，返回 `Container&`）

| 方法 | 签名 | 含义 |
|------|------|------|
| `SetPadding` | `Container& SetPadding(const EdgeInsets& p)` | 内边距。 |
| `SetMargin` | `Container& SetMargin(const EdgeInsets& m)` | 外边距。 |
| `SetBackgroundColor` | `Container& SetBackgroundColor(const Color& c)` | 背景填充。 |
| `SetBorderRadius` | `Container& SetBorderRadius(float r)` | 圆角半径（px），0 为直角。 |

## 示例

```cpp
auto card = std::make_shared<Container>();
card->SetFlexDirection(FlexDirection::kColumn)
    .SetPadding({.left = 16, .top = 12, .right = 16, .bottom = 12})
    .SetBackgroundColor({.r = 255, .g = 255, .b = 255, .a = 255})
    .SetBorderRadius(8.0F);
card->AddChild(title);
card->AddChild(body);
```

等价的流式写法：

```cpp
auto card = std::make_shared<Container>()
    ->SetFlexDirection(FlexDirection::kColumn)
    .SetPadding({.left = 16, .top = 12, .right = 16, .bottom = 12})
    .SetBackgroundColor({255, 255, 255, 255})
    .SetBorderRadius(8.0F)
    .children({title, body})
    .build();
```

## 另见

- [Widget](./widget)
- [Expanded](./expanded)
- [SizedBox](./sized-box)
