# Flex 布局

NeoFlux 使用 Taitank（腾讯开源的 flexbox 布局引擎）计算布局。`Container` 暴露 flexbox 属性，直接映射到 Taitank。

## 基本用法

```cpp
auto col = std::make_shared<Container>();
col->SetFlexDirection(FlexDirection::kColumn)   // 子组件垂直排列
   .SetJustifyContent(HAlign::kCenter)          // 主轴居中
   .SetAlignItems(VAlign::kCenter)              // 交叉轴居中
   .SetPadding({.left = 16, .top = 16, .right = 16, .bottom = 16});
```

## Flex Direction

| 值 | 说明 |
|----|------|
| `kColumn` | 垂直排列（默认） |
| `kRow` | 水平排列 |
| `kRowReverse` | 水平反向排列 |

## 对齐方式

- `SetJustifyContent(HAlign)`：主轴对齐（kLeft / kCenter / kRight / kSpaceBetween / kSpaceAround）
- `SetAlignItems(VAlign)`：交叉轴对齐（kTop / kCenter / kBottom / kStretch）

## 弹性伸缩

- `Expanded`：设置 flex_grow，填充剩余空间
- `SetFlexGrow(float)`：设置弹性增长因子
- `SetFlexShrink(float)`：设置弹性收缩因子

## 固有尺寸

叶子组件（`Text`、`Button`）通过 `OnMeasure()` 报告固有尺寸，Taitank 布局时调用。有 measure 函数的节点不能有子节点。

## 下一步

- [Widget 系统](./widgets)
- [输入与事件](./input)
