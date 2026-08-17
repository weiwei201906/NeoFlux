# SizedBox

固定宽高的容器，用于固定间距或固定尺寸组件。

## 概述

`SizedBox` 继承 `Container`，构造时设置固定宽度和高度。常用于布局中的固定间距。

## 基本用法

```cpp
auto col = std::make_shared<Container>();
col->SetFlexDirection(FlexDirection::kColumn);

col->AddChild(widget1);
col->AddChild(std::make_shared<SizedBox>(0.0F, 16.0F));  // 16px 垂直间距
col->AddChild(widget2);
```

## 构造函数

```cpp
SizedBox(float width, float height);
```

## 方法

继承 `Container` 的所有方法。

## 另见

- [Container](./container)
- [Expanded](./expanded)
