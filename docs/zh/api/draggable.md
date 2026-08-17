# Draggable

可拖拽容器组件，通过指针输入拖拽。拖拽偏移在绘制时应用平移，不影响 Taitank 布局。

## 概述

`Draggable` 继承 `Container`，重写指针事件处理函数以跟踪拖拽状态。用户按下并移动指针时，组件的视觉位置按拖拽增量偏移，布局位置（Taitank 计算）保持不变。

## 状态机

`Draggable` 使用轻量状态机，包含三种状态：

| 状态 | 说明 |
|------|------|
| `kIdle` | 指针未与组件交互 |
| `kHovering` | 指针在组件上方但未按下 |
| `kDragging` | 指针按下并移动组件 |

状态转换由指针事件触发：
- `OnPointerEnter` → `kHovering`（非拖拽时）
- `OnPointerExit` → `kIdle`（非拖拽时）
- `OnPointerDown` → `kDragging`
- `OnPointerUp` → `kIdle`

## 使用示例

```cpp
#include <neoflux/widget/draggable.h>

auto box = std::make_shared<Draggable>();
box->SetWidth(100.0F)
   .SetHeight(100.0F)
   .SetBackgroundColor({.r = 51, .g = 128, .b = 230, .a = 255})
   .SetBorderRadius(12.0F);

auto label = std::make_shared<Text>("Drag Me");
box->AddChild(label);
```

## API 参考

### 方法

| 方法 | 说明 |
|------|------|
| `GetDragOffset() -> Point` | 返回当前拖拽偏移 (x, y) |
| `IsDragging() -> bool` | 返回组件是否正在被拖拽 |

### 重写事件

| 事件 | 行为 |
|------|------|
| `OnPointerDown(pos)` | 记录按下位置，设置拖拽状态 |
| `OnPointerUp(pos)` | 结束拖拽，返回空闲状态 |
| `OnPointerMove(pos)` | 拖拽时更新偏移，标记需要重建 |
| `OnPointerEnter()` | 转换为悬停状态 |
| `OnPointerExit()` | 转换为空闲状态 |
| `Paint(ctx)` | 保存上下文，平移拖拽偏移，绘制子组件，恢复 |

## 长按检测模式

将 `Draggable` 与协程结合实现长按检测。状态机作为"条件锁"：指针按下时启动的协程在睡眠后检查组件状态，如果状态已改变（指针释放），协程静默返回。

```cpp
class DragBox : public Draggable {
 public:
  bool OnPointerDown(const Point& local_pos) override {
    Draggable::OnPointerDown(local_pos);
    auto weak = std::weak_ptr<DragBox>(
        std::static_pointer_cast<DragBox>(shared_from_this()));
    app_->GetEventLoop().Schedule(LongPressCoroutine(weak));
    return true;
  }

 private:
  static Task<void> LongPressCoroutine(std::weak_ptr<DragBox> weak) {
    co_await Sleep(std::chrono::milliseconds(500));
    auto box = weak.lock();
    if (!box || !box->IsDragging()) co_return;
    // 检测到长按
  }
};
```

## 另见

- [Widget](./widget)
- [Container](./container)
- [ScrollView](./scroll-view)
