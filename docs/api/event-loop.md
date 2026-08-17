# EventLoop

CV-driven frame scheduler, coroutine scheduler, and timer queue.

## Overview

`EventLoop` manages the application's main loop, running at a target frame
rate. It supports C++20 coroutine scheduling via `Schedule()`, one-frame
yields via `ScheduleYield()`, and timed resumes via `ScheduleSleep()`.

## Methods

| Method | Description |
|--------|-------------|
| `Run(frame_callback)` | Start the event loop, blocks until `Stop()` |
| `Stop()` | Stop the event loop |
| `WakeUp()` | Wake the loop from idle wait |
| `Schedule(task)` | Schedule a `Task<void>` coroutine |
| `ScheduleYield(handle)` | Resume a handle on the next frame (used by `Yield()`) |
| `ScheduleSleep(duration, handle)` | Resume a handle after a duration (used by `Sleep()`) |
| `SetTargetFps(fps)` | Set target frame rate |
| `GetTargetFps() -> int` | Get target frame rate |
| `GetFrameCount() -> uint64_t` | Get frames processed |

## Coroutine Lifecycle

The loop uses `shared_ptr<Task<void>>` for all scheduled coroutines. An
`active_tasks_` map (keyed by handle address) holds shared ownership, ensuring
a coroutine frame is not destroyed while a timer or yield-pending handle still
references it. See [Coroutines](../guide/coroutines) for details.

## Thread Safety

`Schedule()`, `ScheduleYield()`, and `ScheduleSleep()` are guarded by a mutex
and may be called from any thread. `WakeUp()` is thread-safe.

## See Also

- [Task](./task)
- [Application](./application)
- [Coroutines Guide](../guide/coroutines)
