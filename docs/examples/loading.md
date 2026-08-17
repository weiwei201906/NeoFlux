# Loading Animation

Demonstrates the widget state machine integrated with C++20 coroutines. A "Start Loading" button transitions the widget to a loading state; a coroutine animates a progress bar from 0% to 100% over ~2 seconds, yielding one frame per step. On completion, the widget transitions to a success state.

## Running

```bash
./bin/loading_demo
```

## Key Concepts

### State Machine

The root widget uses `WidgetState` to track three states:

- `kIdle` — initial state, shows "Start Loading" button
- `kLoading` — progress bar animating, shows percentage
- `kSuccess` — loading complete, shows "Done!" message

### Coroutine Animation

The loading animation is driven by a coroutine that yields each frame:

```cpp
Task<void> LoadingCoroutine() {
  for (int i = 0; i <= 100; ++i) {
    progress_ = i / 100.0F;
    MarkNeedsBuild();
    co_await Yield();  // resume next frame
  }
  SetState(WidgetState::kSuccess);
}
```

Each iteration updates the progress value, marks the widget dirty, and yields to the next frame. The event loop resumes the coroutine once per frame.

### Progress Bar

The progress bar uses a `Container` as the track with a child `Container` as the fill. The track uses row flex direction so the fill sits inside it, left-aligned. The fill width is proportional to `progress_`.

## See Also

- [Coroutines Guide](../guide/coroutines)
- [Task API](../api/task)
