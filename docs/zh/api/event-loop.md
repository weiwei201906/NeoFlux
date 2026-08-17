# EventLoop

CV 驱动的帧调度器、协程调度器和定时器队列。

## 概述

`EventLoop` 管理应用主循环，以目标帧率运行。支持通过 `Schedule()` 调度 C++20 协程，通过 `ScheduleYield()` 实现单帧让出，通过 `ScheduleSleep()` 实现定时恢复。

## 方法

| 方法 | 说明 |
|------|------|
| `Run(frame_callback)` | 启动事件循环，阻塞直到 `Stop()` |
| `Stop()` | 停止事件循环 |
| `WakeUp()` | 从空闲等待中唤醒循环 |
| `Schedule(task)` | 调度一个 `Task<void>` 协程 |
| `ScheduleYield(handle)` | 下一帧恢复句柄（由 `Yield()` 使用） |
| `ScheduleSleep(duration, handle)` | 指定时长后恢复句柄（由 `Sleep()` 使用） |
| `SetTargetFps(fps)` | 设置目标帧率 |
| `GetTargetFps() -> int` | 获取目标帧率 |
| `GetFrameCount() -> uint64_t` | 获取已处理帧数 |

## 协程生命周期

循环对所有调度的协程使用 `shared_ptr<Task<void>>`。`active_tasks_` map（以句柄地址为 key）持有共享所有权，确保只要定时器或 yield 等待中的句柄还引用着协程，协程帧就不会被销毁。详见 [协程](../guide/coroutines)。

## 线程安全

`Schedule()`、`ScheduleYield()` 和 `ScheduleSleep()` 由互斥锁保护，可从任意线程调用。`WakeUp()` 线程安全。

## 另见

- [Task](./task)
- [Application](./application)
- [协程指南](../guide/coroutines)
