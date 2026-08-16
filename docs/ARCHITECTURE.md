# NeoFlux Architecture

## Overview

NeoFlux is a cross-platform C++20 UI framework inspired by Flutter's widget
model. It separates UI logic from rendering into two layers connected by a
lock-free ring queue.

## Two-Layer Architecture

### Application Layer (UI Thread)

The application layer is where all business logic and widget tree management
happens. It runs on a single thread (the UI thread) and is responsible for:

1. **Build Phase**: Construct the widget tree. Dirty widgets (marked by
   `setState()` or route changes) are rebuilt via their `Build()` method.

2. **Layout Phase**: Compute widget sizes and positions. Each widget's
   `Layout()` method receives constraints and returns its desired size.
   Taitank (Tencent's flexbox layout engine) can be used for complex
   flex layouts.

3. **Paint Phase**: Record render commands into a `RenderContext`. Widgets
   call methods like `DrawRect()`, `DrawText()`, `Save()`, `Restore()`,
   `Translate()`, and `ClipRect()`.

4. **Submit Phase**: The recorded commands are submitted to the render
   layer via the SPSC ring queue.

### Render Layer (Render Thread)

The render layer runs on a dedicated thread and is responsible for:

1. **Command Consumption**: Pulls `RenderCommand` objects from the ring queue.

2. **Command Execution**: Translates commands into tgfx API calls.

3. **Frame Presentation**: On desktop, swaps GLFW buffers; on mobile,
   presents to the platform surface.

### SPSC Ring Queue

The `SpscRingQueue<T, Capacity>` is a lock-free single-producer single-
consumer bounded ring buffer:

- **Producer**: Application thread (submits render commands).
- **Consumer**: Render thread (executes commands).
- **No mutexes**: Uses `std::atomic` with acquire/release memory ordering.
- **Cache-line aligned**: Head and tail indices are on separate cache lines
  to prevent false sharing.
- **Power-of-two capacity**: Enables fast modulo via bitwise AND.

## Widget System

### Widget Hierarchy

```
Widget (abstract base)
├── StatelessWidget
├── StatefulWidget (owns State)
├── Container
├── Text
└── Button
```

### Widget Lifecycle

1. **Construction**: Widget is created (typically in a `Build()` method or
   route builder).
2. **Build**: `Build(BuildContext&)` is called to produce child widgets.
   For `StatefulWidget`, this delegates to `State::Build()`.
3. **Layout**: `Layout(LayoutConstraints)` computes the widget's size and
   positions its children.
4. **Paint**: `Paint(RenderContext&)` records drawing commands.
5. **Rebuild**: When `MarkNeedsBuild()` is called (via `setState()`), the
   widget is rebuilt in the next frame's build phase.

### State Management

`StatefulWidget` owns a `State` object that persists across rebuilds:

```cpp
class CounterWidget : public StatefulWidget {
  std::unique_ptr<State<StatefulWidget>> CreateState() override {
    return std::make_unique<CounterState>();
  }
};

class CounterState : public State<StatefulWidget> {
  int count_ = 0;
  std::shared_ptr<Widget> Build(BuildContext& ctx) override {
    auto button = std::make_shared<Button>("Count: " + std::to_string(count_));
    button->SetOnPressed([this]() {
      SetState([this]() { ++count_; });
    });
    return button;
  }
};
```

### Route Navigation

Widgets are registered by route name and built on demand:

```cpp
RouteRegistry::Instance().RegisterRoute("/settings", [](BuildContext& ctx) {
  return std::make_shared<SettingsPage>();
});

// Later, from any widget:
ctx.PushRoute("/settings");
ctx.PopRoute();
```

The `Application` maintains a navigation stack; the top widget is the
currently displayed route.

## Rendering Pipeline

### Render Commands

Each frame, the widget tree records a sequence of `RenderCommand` objects:

| Command | Payload | Description |
|---------|---------|-------------|
| `kDrawRect` | Rect, Color | Draw a filled rectangle |
| `kDrawText` | string, Point, Color, float | Draw text at position |
| `kSave` | - | Save transform/clip state |
| `kRestore` | - | Restore previous state |
| `kTranslate` | dx, dy | Translate coordinate origin |
| `kClipRect` | Rect | Set rectangular clip region |
| `kBeginFrame` | - | Start new frame |
| `kEndFrame` | - | End frame (swap buffers) |

### tgfx Integration

`TgfxRenderer` wraps the Tencent tgfx 2D graphics library:

- **Desktop**: tgfx uses an OpenGL context provided by GLFW.
- **Mobile**: tgfx uses the platform surface (ANativeWindow on Android,
  CAMetalLayer on iOS).
- The renderer translates `RenderCommand` objects into tgfx `Canvas` calls
  (`drawRect`, `drawSimpleText`, `save`, `restore`, `translate`, `clipRect`).

### GLFW Bridge (Desktop Only)

`GlfwBridge` provides window management on desktop platforms:

- Creates a native window with an OpenGL 3.3 core profile context.
- Polls input events (keyboard, mouse, window resize).
- Swaps front/back buffers for double-buffered rendering.
- Forwards events to the widget tree for hit-testing and dispatch.

## Thread Safety

| Component | Thread | Mutex? |
|-----------|--------|--------|
| Widget tree | UI thread only | No (single-threaded) |
| RouteRegistry | UI thread (registration at startup) | No |
| SPSC Ring Queue | UI (producer) + Render (consumer) | No (lock-free) |
| tgfx Renderer | Render thread only | No |
| GLFW Window | UI thread (poll) + Render thread (swap) | GLFW context affinity |

The only cross-thread data flow is through the SPSC ring queue, which
guarantees lock-free thread safety for the single-producer single-consumer
pattern.

## Build System

- **CMake 3.20+** with `FetchContent` for all third-party dependencies.
- **C++20** standard required.
- **clang-tidy** integrated via `CMAKE_CXX_CLANG_TIDY`.
- **Google Test** for unit testing, with `gtest_discover_tests`.
- Cross-platform compiler flags in `cmake/CompilerFlags.cmake`.
- Sanitizer support (ASan, TSan) via CMake options.
