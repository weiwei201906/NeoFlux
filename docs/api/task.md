# Task (Coroutine)

C++20 coroutine type for frame-based asynchronous work.

## Header

```cpp
#include <neoflux/core/task.h>
```

## Overview

`Task<T>` is a lazy coroutine. It starts suspended and only begins execution
when scheduled on the `EventLoop`.

## Basic Usage

```cpp
neoflux::Task<void> MyCoroutine() {
  LOG(INFO) << "Step 1";
  co_await neoflux::Yield();  // suspend, resume next frame
  LOG(INFO) << "Step 2";
}

// Schedule on the event loop
app.GetEventLoop().Schedule(MyCoroutine());
```

## Awaitables

### `Yield()`

```cpp
co_await neoflux::Yield();
```

Suspends the coroutine and resumes it on the next frame. Use for frame-based
animations and timed sequences.

## Returning Values

```cpp
neoflux::Task<int> ComputeAsync() {
  co_await neoflux::Yield();
  co_return 42;
}
```

## Animation Example

```cpp
neoflux::Task<void> FadeIn(std::weak_ptr<Widget> weak_widget) {
  for (int frame = 0; frame <= 30; ++frame) {
    auto widget = weak_widget.lock();
    if (!widget) co_return;
    const float opacity = static_cast<float>(frame) / 30.0F;
    widget->SetOpacity(opacity);
    widget->MarkNeedsBuild();
    co_await neoflux::Yield();
  }
}
```

## Implementation Notes

- `Task<T>` contains a `promise_type` with `initial_suspend` = `suspend_always`
  (lazy start).
- `final_suspend` = `suspend_always` (manual lifetime management).
- The `EventLoop` calls `Resume()` on each pending task; if it returns `false`,
  the task is requeued for the next frame.
- Coroutines are moved into the event loop's pending queue.

## When to Use

- Frame-based animations and transitions
- Multi-step UI flows with delays
- Async operations that yield to the event loop
- State machines that span multiple frames
