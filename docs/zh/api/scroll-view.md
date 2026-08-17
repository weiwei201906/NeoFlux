# ScrollView

可滚动视口组件，支持滚轮与拖拽滚动，内容自动裁剪。

## 概述

`ScrollView` 继承 `Container`，提供垂直滚动功能。内容超出视口时可通过鼠标滚轮或拖拽滚动。

## 基本用法

```cpp
auto scroll = std::make_shared<ScrollView>();
scroll->SetWidth(300.0F)
    .SetHeight(400.0F);

// 添加内容
for (int i = 0; i < 20; ++i) {
  auto item = std::make_shared<Container>();
  item->SetHeight(40.0F);
  scroll->AddChild(item);
}
```

## 滚动方式

- **鼠标滚轮**：`OnPointerScroll` 处理滚轮事件
- **拖拽滚动**：按下并拖动内容区域滚动

## 状态机

ScrollView 内部使用轻量状态机跟踪滚动状态：
- `kIdle`：空闲
- `kDragging`：正在拖拽滚动

## 方法

| 方法 | 说明 |
|------|------|
| `SetContent(child)` | 设置滚动内容 |
| `ScrollTo(y)` | 滚动到指定位置 |
| `GetScrollY() -> float` | 获取当前滚动偏移 |

## 另见

- [Widget](./widget)
- [Container](./container)
- [滚动视图示例](../examples/scroll)
