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

The coroutine resumes on each frame when it is ready.

## Yield

`Yield()` suspends the coroutine and resumes it on the next frame:

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

## Animation Example

```cpp
class AnimatedBox : public StatefulWidget {
 public:
  void StartAnimation(EventLoop& loop) {
    loop.Schedule(Animate());
  }

 private:
  neoflux::Task<void> Animate() {
    for (int frame = 0; frame < 120; ++frame) {
      const float t = static_cast<float>(frame) / 120.0F;
      offset_ = 200.0F * t;
      MarkNeedsBuild();
      co_await neoflux::Yield();
    }
  }

  float offset_ = 0.0F;
};
```

## Coroutine-Aware Event Loop

The `EventLoop` runs ready coroutines each frame:

```cpp
void EventLoop::RunReadyCoroutines() {
  std::vector<Task<void>> ready;
  {
    std::lock_guard lock(coroutine_mutex_);
    ready.swap(pending_coroutines_);
  }
  for (auto& task : ready) {
    if (!task.Resume()) {
      // still pending, requeue
      std::lock_guard lock(coroutine_mutex_);
      pending_coroutines_.push_back(std::move(task));
    }
  }
}
```

## When to Use Coroutines

- **Animations**: Frame-based transitions, easing curves.
- **Timed sequences**: Multi-step UI flows with delays.
- **Async I/O**: Yield while waiting for resources.
- **State machines**: Coroutines can model complex state transitions naturally.

## When Not to Use Coroutines

- Simple per-frame updates that can be done in `OnFrame`.
- High-frequency logic where coroutine overhead matters.
- Operations that don't need to span multiple frames.
