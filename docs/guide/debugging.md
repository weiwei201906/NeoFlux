# Debugging & Tuning

How to get diagnostics out of NeoFlux, read its logs, and tune the runtime
behaviour. Most knobs are gflags (runtime); a smaller set lives in
`neoflux/include/neoflux/core/config.h` (compile time).

## Logging basics

NeoFlux uses [glog](https://github.com/google/glog). By default logs are
written to `./logs/*.log` (the directory is created if missing). Because the
desktop binary is a GUI-subsystem executable with no console, you usually want
to mirror logs to stderr while developing:

```powershell
.\build\bin\neoflux_app.exe --logtostderr
```

::: tip Always pass --logtostderr during development
Without it, the GUI app opens no console at all and you will not see any
`LOG(ERROR)` output. Files under `./logs/` do appear, but tailing them in
another window is slower than reading them on the terminal.
:::

| Flag | Default | Purpose |
| --- | --- | --- |
| `--logtostderr` | off | Mirror all logs to stderr. |
| `--log_dir` | `./logs` | Where `.log` files go. |
| `--verbose_logging` | off | Turns on `VLOG(1)` (per-frame layout dump). |

### What to look for

- `FontManager scanned <dir>: found N fonts` — confirms fonts were picked up.
- `Render command queue full, dropped K commands` — back-pressure, see below.
- `mpv` lines (when media is on) — decoder / GPU upload state.

`VLOG(1)` prints one line per laid-out widget per frame. It is noisy but the
fastest way to find a widget whose geometry is wrong:

```powershell
.\build\bin\neoflux_app.exe --logtostderr --verbose_logging
```

## Render queue back-pressure

The App thread emits `RenderCommand`s into a single-producer single-consumer
ring queue; the render thread drains it. If the producer out-runs the consumer
(say, a frame builds thousands of widgets), the queue fills and excess commands
are dropped:

```
WARNING: Render command queue full, dropped 14 commands
```

Diagnosis and tuning:

| Flag | Default | What it does |
| --- | --- | --- |
| `--render_queue_capacity` | `2048` | Ring slots. Rounded up to a power of two; one slot reserved, so usable = `capacity - 1`. |
| `--render_queue_drop_log_max` | `10` | How many "queue full" warnings to print per process before going silent. Raise this when you are actively chasing back-pressure. |

::: warning Do not blindly raise the queue
A larger queue increases memory and present latency. If you see drops, first
look for a widget tree that rebuilds every frame when it could rebuild on
state change (`MarkNeedsBuild`), or a paint path that allocates per frame.
:::

## Frame loop & wake-ups

NeoFlux does **not** busy-poll. The event loop sleeps between frames and is
woken by:

- input events (pointer, key),
- `Application::MarkFrameDirty()` from widget code,
- mpv frame callbacks (when media is playing).

`--target_fps` caps the maximum rate; the loop still sleeps shorter when
something wakes it early.

::: tip If the screen does not update until you move the mouse
You forgot to call `MarkNeedsBuild()` / `MarkFrameDirty()` after a state change.
Widgets that own mutable state must re-dirty themselves; the framework does not
guess.
:::

## Compile-time knobs

`neoflux/include/neoflux/core/config.h` holds values you might want to retune
when porting:

| Constant | Default | When to change |
| --- | --- | --- |
| `config::kCacheLineSize` | `64` | On CPUs with 128-byte lines (some Apple M, newer AMD) define `NEOFLUX_CACHE_LINE_SIZE=128` to avoid false sharing. |
| `config::kDefaultRenderQueueCapacity` | `2048` | Overridden by the `--render_queue_capacity` gflag. |
| `config::kLongPressThresholdMs` | `500` | Button long-press threshold. |

## Code style: what not to do

### Do not abuse the fluent builder for control flow

The fluent chain exists to make *declarative* widget construction read cleanly.
It is not a replacement for `if`/`for`:

```cpp
// Bad: burying a conditional deep in a chain.
root->SetPadding(...).AddChild(title);
if (show_button) root->AddChild(btn);   // OK, just don't do this:
root->AddChild(show_button ? btn : nullptr);  // nullptr children are a bug.
```

::: warning Fluent is for one-shot construction
Each `SetXxx()` returns `*this` for chaining. Do not keep a fluent chain
across lines that also mutate local state; rebuild the tree in a function and
return it. Long chains of `.AddChild(...).AddChild(...)` are fine; chains that
branch on runtime state are not.
:::

### Do not allocate in `Paint()`

`Paint()` runs once per frame per visible widget. Allocating memory there
(new `std::string`, `std::vector`, `std::function`) triggers the allocator on
the hot path. Cache geometry and strings on the widget, not in `Paint`.

### Do not touch GL / mpv / native handles from widget code

Widget `Paint()` only emits `RenderCommand`s. Actual GL work happens on the
render thread. Calling `gl*` directly from a widget is a cross-thread violation
and will crash on OpenGL contexts that are not current.

### Prefer `std::shared_ptr<Widget>` (never raw `new`)

The widget tree is owned by `shared_ptr`. Construct with `std::make_shared<T>()`
and never store a raw `T*` into another widget — use `std::weak_ptr` for
back-references so a destroyed widget does not leave a dangling callback.

```cpp
// Good: lambda captures the child as shared_ptr, the button owns it.
auto play_btn = std::make_shared<Button>("Play");
play_btn->SetOnPressed([media, play_btn]() { /* ... */ });

// Bad: capturing `this` in a long-lived callback without a weak guard.
toggle->SetOnPressed([this] { state_ = !state_; });  // safe only if the
  // widget outlives the callback; for async/coroutine callbacks, use weak_ptr.
```

## Quick checklist

1. Black window? `--logtostderr` first — likely a missing font directory or a
   backend that is not available.
2. Logs say "queue full"? Check for per-frame widget rebuilds; tune
   `--render_queue_capacity` only as a band-aid.
3. Screen does not refresh until mouse moves? You forgot to mark the tree dirty.
4. Cross-thread crash? You called GL/mpv from the App thread (or vice versa).
   Re-read the [media threading model](./media.md#threading-model).
5. CJK glyphs missing? Point `assets/fonts/` at a CJK font, or check
   [Font System](./fonts.md).
