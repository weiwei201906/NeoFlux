# 协程

NeoFlux 支持 C++20 协程用于异步工作。

## 基本用法

在事件循环上调度一个 `Task<void>`，它在就绪时的下一帧恢复：

```cpp
#include <neoflux/core/task.h>

neoflux::Task<void> AnimateAsync() {
  for (int i = 0; i < 60; ++i) {
    co_await neoflux::Yield();  // 下一帧恢复
    widget->SetOpacity(i / 60.0F);
  }
}

event_loop.Schedule(AnimateAsync());
```

## Sleep

使用 `co_await Sleep(duration)` 将协程挂起指定时长。事件循环维护定时器队列，每帧检查并恢复到期的定时器：

```cpp
neoflux::Task<void> LongPressDetector(std::weak_ptr<Button> weak_btn) {
  co_await neoflux::Sleep(std::chrono::milliseconds(500));
  auto btn = weak_btn.lock();
  if (!btn) co_return;          // 组件已销毁
  if (btn->IsPressed()) {       // 状态机作为条件锁
    btn->OnLongPress();
  }
}
```

## 状态机 + 协程模式

Widget 携带轻量 `WidgetState`（Idle、Hovering、Dragging 等）。状态迁移是协程的"条件锁"：指针按下时启动的协程在睡眠后检查 Widget 状态，如果状态已改变，协程静默返回。无需显式取消机制。

## 下一步

- [Task API](../api/task)
- [加载动画示例](../examples/loading)
