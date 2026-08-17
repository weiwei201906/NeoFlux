# Button

可点击按钮组件，支持标签、按下回调和按下状态样式。

## 概述

`Button` 继承 `Container`，重写指针事件处理按下/释放状态，并在释放时触发回调。

## 基本用法

```cpp
auto button = std::make_shared<Button>("Click Me");
button->SetOnPressed([]() {
  // 处理点击
});
```

## 方法

| 方法 | 说明 |
|------|------|
| `SetOnPressed(callback)` | 设置按下回调 |
| `SetLabel(text)` | 设置按钮标签文本 |
| `GetLabel() -> std::string_view` | 获取按钮标签 |

## 样式

Button 继承 Container 的所有样式方法：
- `SetBackgroundColor(color)`
- `SetBorderRadius(radius)`
- `SetWidth(width)` / `SetHeight(height)`
- `SetPadding(insets)`

## 事件

Button 重写以下事件：
- `OnPointerDown(pos)`：设置按下状态
- `OnPointerUp(pos)`：恢复正常状态，触发回调

## 另见

- [Widget](./widget)
- [Container](./container)
