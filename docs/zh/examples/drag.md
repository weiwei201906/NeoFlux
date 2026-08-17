# 拖拽演示

演示 `Draggable` 组件与指针事件，以及"状态机作为条件锁"模式。彩色方块可拖拽；状态标签显示当前状态（Idle/Hovering/Dragging）和偏移量。

## 运行

```bash
./bin/drag_demo
```

## 核心概念

### Draggable 组件

`Draggable` 组件继承 `Container`，处理指针事件以跟踪拖拽状态。拖拽偏移在绘制时通过 `context.Translate()` 应用，不影响 Taitank 布局。

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
