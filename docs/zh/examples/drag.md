# 拖拽演示

演示 `Draggable` 组件与指针事件，以及"状态机作为条件锁"模式。彩色方块可拖拽；状态标签显示当前状态（Idle/Hovering/Dragging）和偏移量。

## 运行

```bash
./bin/drag_demo
```

## 核心概念

### Draggable 组件

`Draggable` 组件继承 `Container`，处理指针事件以跟踪拖拽状态。拖拽偏移在绘制时通过 `context.Translate()` 应用，不影响 Taitank 布局。

**中心跟随光标行为：** 组件中心始终跟随鼠标光标。指针按下时，组件立即跳转使其中心位于点击点。移动过程中，`drag_offset_ += local_pos - bounds.size / 2` 保持中心在光标上。这不需要 `MarkNeedsBuild()`——仅需事件分发调用的 `MarkFrameDirty()` 触发重绘。

### 绘制时变换与命中测试

由于 `Draggable` 应用绘制时平移，三个机制保持坐标一致：

1. `GetPaintOffset()` 返回 `drag_offset_`，使事件分发将视觉坐标转换为局部坐标。
2. `HitTest()` 在基类测试前减去 `drag_offset_`，使视觉位置的点击正确命中。
3. `DispatchPointerEvent/Move` 在计算 `local_pos` 时减去 `GetPaintOffset()`。

### 状态机

组件在三种状态间转换：
- `kIdle` — 无交互
- `kHovering` — 指针在组件上方
- `kDragging` — 指针按下并移动

### 长按检测

指针按下时启动长按检测协程。协程使用 `weak_ptr` 引用组件，500ms 后检查拖拽状态。如果超时前指针已释放，协程观察到状态变化后静默返回。如果按住超过 500ms，显示 "[Long Press!]" 指示器。

```cpp
Task<void> LongPressCoroutine(std::weak_ptr<DragBox> weak) {
  co_await Sleep(std::chrono::milliseconds(500));
  auto box = weak.lock();
  if (!box) co_return;              // 组件已销毁
  if (!box->IsDragging()) co_return; // 超时前已释放
  box->SetLongPressFired(true);
  box->MarkNeedsBuild();
}
```

这就是"状态机作为条件锁"模式：无需显式取消机制——状态机本身就是协程执行的门控。

### 命中测试缓存

指针移动事件使用命中测试缓存，避免每次移动都遍历整棵 Widget 树。布局变化时缓存自动失效。

## 另见

- [Draggable API](../api/draggable)
- [输入与事件指南](../guide/input)
- [协程指南](../guide/coroutines)
