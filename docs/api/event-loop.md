# EventLoop

The event loop drives the application's frame cycle and coroutine scheduling.

## Header

```cpp
#include <neoflux/app/event_loop.h>
```

## Methods

### `Run()`

```cpp
void Run(std::function<void()> frame_callback);
```

Runs the event loop. The `frame_callback` is invoked each frame (when dirty).
Blocks until `Stop()` is called.

### `Stop()`

```cpp
void Stop();
```

Stops the event loop.

### `WakeUp()`

```cpp
void WakeUp();
```

Wakes the event loop to process a frame immediately.

### `Schedule()`

```cpp
void Schedule(Task<void> coroutine);
```

Schedules a C++20 coroutine to run. The coroutine resumes on subsequent frames
when ready.

## Frame Cycle

Each frame:

1. Poll platform events (input, window events).
2. Run ready coroutines (`RunReadyCoroutines()`).
3. Invoke the frame callback (build → layout → paint → submit).
4. Wait for the next frame (target FPS via `--target_fps`).

## Coroutine Integration

```cpp
EventLoop& loop = app.GetEventLoop();

loop.Schedule([]() -> neoflux::Task<void> {
  for (int i = 0; i < 60; ++i) {
    co_await neoflux::Yield();
    // update animation state
  }
}());
```

The event loop maintains a queue of pending coroutines. Each frame, it resumes
all ready coroutines and requeues those that are still pending.
