# Coroutines

NeoFlux supports C++20 coroutines for frame-based asynchronous work. This is
useful for animations, timed sequences, and async operations that need to yield
to the event loop.

## Task Type

`Task<T>` is a lazy coroutine type. It starts suspended and only begins
execution when scheduled on the event loop.

```cpp
#include <neoflux/core/task.h>

neoflux::Task<void> MyCoroutine() {
  LOG(INFO) << "Step 1";
  co_await neoflux::Yield();  // suspend, resume next frame
  LOG(INFO) << "Step 2 (next frame)";
}
```

## Scheduling

Schedule a coroutine on the application's event loop:

```cpp
Application app;
// ... init ...
app.GetEventLoop().Schedule(MyCoroutine());
```

The coroutine is wrapped in a `shared_ptr` and registered in an `active_tasks_`
map keyed by coroutine handle address. This ensures the coroutine frame stays
alive for as long as a timer or yield-pending handle references it, preventing
use-after-free when a `Sleep` coroutine is resumed after its owning `Task` would
have gone out of scope.

## Yield

`Yield()` suspends the coroutine and registers it with the event loop's
yield queue. It resumes on the next frame:

```cpp
neoflux::Task<void> FadeIn(Widget* widget) {
  for (int i = 0; i <= 60; ++i) {
    const float opacity = static_cast<float>(i) / 60.0F;
    widget->SetOpacity(opacity);
    co_await neoflux::Yield();  // one frame per iteration
  }
}
```

This runs a 60-frame fade-in animation at ~60 FPS.

## Sleep

`Sleep(duration)` suspends the coroutine for the specified duration using the
event loop's timer queue. The coroutine is **not** resumed each frame while
waiting; it is only resumed when the timer expires.

```cpp
neoflux::Task<void> LongPressDetector(std::weak_ptr<Button> weak_btn) {
  co_await neoflux::Sleep(std::chrono::milliseconds(500));
  auto btn = weak_btn.lock();
  if (!btn) co_return;          // widget destroyed
  if (btn->IsPressed()) {       // state machine as condition lock
    btn->OnLongPress();
  }
}
```

## Lifecycle Management

The event loop maintains three coroutine collections:

| Collection | Purpose |
|------------|---------|
| `pending_coroutines_` | Tasks ready to resume this frame |
| `active_tasks_` | All live tasks, keyed by handle address (shared ownership) |
| `yield_handles_` | Handles that requested `co_await Yield()` |
| `timer_queue_` | `multimap<time_point, handle>` for `co_await Sleep()` |

Each frame `RunReadyCoroutines()` executes four phases:

1. **Promote yields**: move yield-pending tasks back to `pending_coroutines_`
2. **Resume pending**: swap and resume all pending tasks
3. **Collect completed**: erase finished tasks from `active_tasks_`
4. **Fire timers**: look up each expired timer's owning `Task` from
   `active_tasks_` before resuming — this prevents resuming a dangling handle

A task is only destroyed when its `shared_ptr` refcount reaches zero, which
happens after it completes and is erased from `active_tasks_` and all local
references go out of scope.

## State Machine + Coroutine Pattern

Widgets carry a lightweight `WidgetState` (Idle, Hovering, Dragging, etc.).
State transitions act as a "condition lock" for coroutines: a long-press
coroutine started on pointer-down checks the widget's state after sleeping;
if the state has changed (e.g. pointer released), the coroutine silently
returns. No explicit cancellation mechanism is needed.

## When to Use Coroutines

- **Animations**: Frame-based transitions, easing curves.
- **Timed sequences**: Multi-step UI flows with delays.
- **Async I/O**: Yield while waiting for resources.
- **State machines**: Coroutines can model complex state transitions naturally.

## When Not to Use Coroutines

- Simple per-frame updates that can be done in `OnFrame`.
- High-frequency logic where coroutine overhead matters.
- Operations that don't need to span multiple frames.
