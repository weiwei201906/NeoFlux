# Container

Flexbox 容器组件，支持 padding、margin、背景色、圆角和 flex 布局。

## 概述

`Container` 继承 `Widget`，是最常用的布局容器。它暴露 Taitank flexbox 属性，用于组织子组件。

## 基本用法

```cpp
auto container = std::make_shared<Container>();
container->SetFlexDirection(FlexDirection::kColumn)
    .SetJustifyContent(HAlign::kCenter)
    .SetAlignItems(VAlign::kCenter)
    .SetPadding({.left = 16, .top = 16, .right = 16, .bottom = 16})
    .SetBackgroundColor({.r = 245, .g = 245, .b = 250, .a = 255})
    .SetBorderRadius(8.0F);

container->AddChild(child1);
container->AddChild(child2);
```

## 布局方法

| 方法 | 说明 |
|------|------|
| `SetFlexDirection(direction)` | 设置 flex 方向（kColumn / kRow / kRowReverse） |
| `SetJustifyContent(align)` | 设置主轴对齐 |
| `SetAlignItems(align)` | 设置交叉轴对齐 |
| `SetPadding(insets)` | 设置内边距 |
| `SetMargin(insets)` | 设置外边距 |
| `SetWidth(width)` / `SetHeight(height)` | 设置固定尺寸 |
| `SetFlexGrow(value)` | 设置弹性增长 |
| `SetFlexShrink(value)` | 设置弹性收缩 |

## 样式方法

| 方法 | 说明 |
|------|------|
| `SetBackgroundColor(color)` | 设置背景色 |
| `SetBorderRadius(radius)` | 设置圆角 |

## 另见

- [Widget](./widget)
- [Expanded](./expanded)
- [SizedBox](./sized-box)
