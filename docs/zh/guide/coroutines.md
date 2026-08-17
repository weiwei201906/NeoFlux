# 协程

NeoFlux 支持 C++20 协程用于帧级异步工作，适用于动画、定时序列和需要让出事件循环的异步操作。

## Task 类型

`Task<T>` 是惰性协程类型。它初始挂起，只有在事件循环上调度后才开始执行。

```cpp
#include <neoflux/core/task.h>

neoflux::Task<void> MyCoroutine() {
  LOG(INFO) << "Step 1";
  co_await neoflux::Yield();  // 挂起，下一帧恢复
  LOG(INFO) << "Step 2 (next frame)";
}
```

## 调度

在应用的事件循环上调度协程：

```cpp
Application app;
// ... init ...
app.GetEventLoop().Schedule(MyCoroutine());
```

协程被包装为 `shared_ptr` 并注册到 `active_tasks_` map（以协程句柄地址为 key）。这确保只要定时器或 yield 等待中的句柄还引用着协程，协程帧就不会被销毁，防止 `Sleep` 协程在其所属 `Task` 出作用域后被恢复时出现 use-after-free。

## Yield

`Yield()` 挂起协程并将其注册到事件循环的 yield 队列，下一帧恢复：

```cpp
neoflux::Task<void> FadeIn(Widget* widget) {
  for (int i = 0; i <= 60; ++i) {
    const float opacity = static_cast<float>(i) / 60.0F;
    widget->SetOpacity(opacity);
    co_await neoflux::Yield();  // 每次迭代一帧
  }
}
```

以约 60 FPS 执行 60 帧淡入动画。

## Sleep

`Sleep(duration)` 使用事件循环的定时器队列挂起协程指定时长。等待期间协程**不会**被每帧恢复，仅在定时器到期时恢复。

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

:::warning
切勿在协程中捕获裸 `Widget*`。始终使用 `std::weak_ptr`，并在每次 `co_await` 后重新 lock。协程挂起期间 Widget 可能被销毁。
:::

## 生命周期管理

事件循环维护四个协程集合：

| 集合 | 用途 |
|------|------|
| `pending_coroutines_` | 本帧准备恢复的任务 |
| `active_tasks_` | 所有活跃任务，以句柄地址为 key（共享所有权） |
| `yield_handles_` | 请求了 `co_await Yield()` 的句柄 |
| `timer_queue_` | `multimap<time_point, handle>`，用于 `co_await Sleep()` |

每帧 `RunReadyCoroutines()` 执行四个阶段：

1. **提升 yield**：将等待 yield 的任务移回 `pending_coroutines_`
2. **恢复 pending**：swap 并恢复所有待处理任务
3. **收集完成**：从 `active_tasks_` 中擦除已完成的任务
4. **触发定时器**：从 `active_tasks_` 查找每个到期定时器所属的 `Task` 后再恢复——防止恢复悬空句柄

任务仅在 `shared_ptr` 引用计数归零时销毁，即完成后从 `active_tasks_` 擦除且所有局部引用出作用域之后。

## 状态机 + 协程模式

Widget 携带轻量 `WidgetState`（Idle、Hovering、Dragging 等）。状态迁移作为协程的"条件锁"：指针按下时启动的长按协程在睡眠后检查 Widget 状态，如果状态已改变（如指针释放），协程静默返回。无需显式取消机制。

## 适用场景

- **动画**：帧级过渡、缓动曲线
- **定时序列**：带延迟的多步 UI 流程
- **异步 I/O**：等待资源时让出
- **状态机**：协程可自然建模复杂状态迁移

## 不适用场景

- 可在 `OnFrame` 中完成的简单逐帧更新
- 协程开销不可忽略的高频逻辑
- 不需要跨多帧的操作
