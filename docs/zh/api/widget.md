# Widget

所有 UI 组件的抽象基类。

## 概述

`Widget` 是 NeoFlux Widget 系统的基类。每个 Widget 绑定一个 Taitank 布局节点，支持子组件、事件处理和自定义绘制。

## 核心方法

| 方法 | 说明 |
|------|------|
| `Build(ctx) -> std::shared_ptr<Widget>` | 构建子组件树，默认返回 nullptr |
| `Paint(ctx)` | 自定义绘制，默认绘制背景和子组件 |
| `OnMeasure(width, height) -> Size` | 报告固有尺寸，叶子组件必须重写 |
| `AddChild(child)` | 添加子组件 |
| `ClearChildren()` | 清除所有子组件 |
| `GetChildren() -> const vector&` | 获取子组件列表 |
| `MarkNeedsBuild()` | 标记需要重建 |
| `NeedsBuild() -> bool` | 是否需要重建 |
| `ClearNeedsBuild()` | 清除重建标记 |

## 事件处理

| 方法 | 说明 |
|------|------|
| `OnPointerDown(pos) -> bool` | 指针按下 |
| `OnPointerUp(pos)` | 指针释放 |
| `OnPointerMove(pos) -> bool` | 指针移动 |
| `OnPointerEnter()` | 指针进入 |
| `OnPointerExit()` | 指针离开 |

## 状态

| 方法 | 说明 |
|------|------|
| `SetState(state)` | 设置 Widget 状态，触发 OnStateChanged |
| `GetState() -> WidgetState` | 获取当前状态 |
| `OnStateChanged(from, to)` | 状态变化回调 |

## 布局属性

- `SetFlexDirection(direction)`
- `SetJustifyContent(align)`
- `SetAlignItems(align)`
- `SetPadding(insets)`
- `SetMargin(insets)`
- `SetWidth(width)` / `SetHeight(height)`
- `SetBackgroundColor(color)`
- `SetBorderRadius(radius)`

## 另见

- [Container](./container)
- [StatefulWidget](./stateful)
