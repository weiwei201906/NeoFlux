# Task (协程)

C++20 协程支持，提供 `Task<T>` 类型、`Yield()` 和 `Sleep()` 等待器。

## 概述

`Task<void>` 是 NeoFlux 的协程返回类型，可在 EventLoop 上调度。协程初始挂起，由事件循环逐帧恢复。

## 基本用法

```cpp
#include <neoflux/core/task.h>

neoflux::Task<void> Animate() {
  for (int i = 0; i < 60; ++i) {
    co_await neoflux::Yield();  // 下一帧恢复
    // 更新状态
  }
}

event_loop.Schedule(Animate());
```

## Sleep

```cpp
neoflux::Task<void> DelayedAction() {
  co_await neoflux::Sleep(std::chrono::milliseconds(500));
  // 500ms 后执行
}
```

## 生命周期

- 协程初始挂起（`initial_suspend` = suspend_always）
- `Schedule()` 将 Task 移入事件循环的待运行队列
- 每帧 `RunReadyCoroutines()` 恢复未完成的协程
- 协程完成后挂起在 `final_suspend`，由 Task 析构函数销毁

## 另见

- [EventLoop](./event-loop)
- [协程指南](../guide/coroutines)
