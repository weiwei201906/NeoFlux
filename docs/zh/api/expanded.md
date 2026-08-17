# Expanded

设置了 flex_grow 的容器，用于填充父容器剩余空间。

## 概述

`Expanded` 继承 `Container`，构造时自动设置 `flex_grow = 1.0`，使其在 flex 布局中填充剩余空间。

## 基本用法

```cpp
auto row = std::make_shared<Container>();
row->SetFlexDirection(FlexDirection::kRow);

auto fixed = std::make_shared<SizedBox>(100.0F, 50.0F);
auto expanded = std::make_shared<Expanded>();
expanded->SetBackgroundColor({.r = 100, .g = 200, .b = 100, .a = 255});

row->AddChild(fixed);
row->AddChild(expanded);  // 填充剩余宽度
```

## 方法

继承 `Container` 的所有方法。

## 另见

- [Container](./container)
- [SizedBox](./sized-box)
