# EventLoop

事件循环类，CV 驱动的帧调度，协程调度器，定时器队列。

## 概述

`EventLoop` 管理应用的主循环，以目标帧率运行帧回调。支持 C++20 协程调度和定时器。

## 方法

| 方法 | 说明 |
|------|------|
| `Run(frame_callback)` | 启动事件循环，阻塞直到 Stop() 被调用 |
| `Stop()` | 停止事件循环 |
| `WakeUp()` | 唤醒等待中的事件循环 |
| `Schedule(task)` | 调度一个 `Task<void>` 协程 |
| `ScheduleSleep(duration, handle)` | 注册一个定时器，到期后恢复协程 |
| `SetTargetFps(fps)` | 设置目标帧率 |
| `GetTargetFps() -> int` | 获取目标帧率 |
| `GetFrameCount() -> uint64_t` | 获取已渲染帧数 |

## 协程支持

- `Yield()`：挂起协程，下一帧恢复
- `Sleep(duration)`：挂起协程，指定时长后恢复

## 线程安全

`Schedule()` 和 `ScheduleSleep()` 使用互斥锁保护，可从其他线程调用。

## 另见

- [Task](./task)
- [Application](./application)
