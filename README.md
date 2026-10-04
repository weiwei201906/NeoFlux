# NeoFlux
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/compiler_support)
[![License](https://img.shields.io/badge/license-GPLv3-green.svg)](LICENSE)

A cross-platform C++20 UI framework with a Flutter-like widget model.

> Licensed under the **GNU General Public License v3.0 (GPL-3.0)**. See [LICENSE](LICENSE) for the full text.
>
> 中文文档请参阅 [README-zh.md](README-zh.md)。

## Architecture

NeoFlux uses a two-layer architecture with lock-free inter-thread communication:

```
+---------------------------+         +---------------------------+
|   Application Layer       |  SPSC   |   Render Layer            |
|   (UI Thread)             |  Ring   |   (Render Thread)         |
|                           |  Queue  |                           |
|  - Widget Tree            | +-----> |  - tgfx Renderer          |
|  - Taitank Layout Engine  |         |  - GLFW Bridge (desktop)  |
|  - Event Loop             |         |  - Command Execution      |
|  - Route Navigation       |         |                           |
+---------------------------+         +---------------------------+
```

- **Application Layer**: Runs business logic, builds the widget tree, computes
  layout via Taitank, and records render commands.
- **Render Layer**: Consumes commands from the SPSC ring queue and executes
  them using tgfx (mobile) or a GLFW+OpenGL bridge (desktop).
- **SPSC Ring Queue**: Lock-free single-producer single-consumer FIFO queue
  connecting the two layers without mutex contention.

## Features

- Flutter-like widget system (StatelessWidget, StatefulWidget, State)
- Route-based navigation with widget registration
- Cross-platform: see [Platform Support](#platform-support) below (Linux / Windows are CI-verified)
- C++20 standard with modern features (std::string_view, ranges, concepts)
- Google C++ style guide compliance
- clang-tidy static analysis
- GLog logging + GFlags command-line parsing
- GTest unit testing
- CMake build system with FetchContent dependency management

## Platform Support

| Platform | Status | Notes |
|----------|--------|-------|
| Linux (x86-64) | ✅ Verified | CI build + headless tests via Xvfb |
| Windows (MSVC x64) | ✅ Verified | CI build + tests |
| macOS | 🚧 Adapted, not CI-verified | Uses the GLFW + OpenGL path |
| Android | 🚧 Renderer + input wired, no app shell | tgfx `EGLWindow` owns the EGL context and renders into the `ANativeWindow`; `MobileBridge` dispatches touch input into the widget tree. An Android app shell (NativeActivity/JNI) that creates the surface and forwards touch events is still needed. |
| iOS | 🚧 Shell required | The `EAGLWindow::MakeFrom(CAEAGLLayer*)` path is defined but the ObjC++ app shell (view hierarchy) does not exist in this repository; `TgfxRenderer::Init` fails with an explicit log. |

> "Verified" means the platform builds and passes tests in CI. Mobile rendering
> and touch input are wired through tgfx's own platform abstractions (EGL/EAGL
> Window) and the `PlatformBridge` input bridge; what is missing is the
> platform application shell (an Android NativeActivity/JNI project, an iOS
> ObjC++ app project) that owns the lifecycle and hands over the native
> surface — see [Mobile Rendering](#mobile-rendering). Media playback on
> mobile returns `nullptr` from `CreateMediaPlayer()` with a loud warning.

### Standalone verification suite

[`verify/`](./verify) contains ten self-contained unit tests that build with a
plain `g++` invocation — no CMake, tgfx, mpv or taitank required. See
[verify/README.md](./verify/README.md) for the exact commands and expected
results.

## Quick Start

### Prerequisites

- CMake 3.20+
- C++20 compatible compiler (GCC 11+, Clang 14+, MSVC 2022+)
- Ninja or Make / Visual Studio

### Build

```bash
mkdir build && cd build
cmake .. -G Ninja
cmake --build .
```

### Run Tests

```bash
ctest --output-on-failure
```

### Run Example

```bash
./bin/hello_neoflux
```

## Configuration (gflags)

NeoFlux uses gflags for runtime configuration. All flags are optional.

| Flag | Type | Default | Description |
|------|------|---------|-------------|
| `--target_fps` | int | `60` | Target frame rate for the application event loop and render pacing. |
| `--idle_fps` | int | `15` | Idle heart-beat rate. When no render request and no coroutine/timer work is pending for a few frames, the event loop drops to this rate to save CPU (input events wake it instantly). `0` disables idle throttling. |
| `--render_queue_capacity` | int | `2048` | Capacity of the SPSC lock-free ring queue between the application and render layers. Rounded up to the next power of two automatically. |
| `--render_queue_drop_log_max` | int | `10` | Maximum number of "queue full, commands dropped" warnings emitted before drops are counted silently. |
| `--native_tuning` | bool | `true` | Master switch for the platform-native tuning layer (thread scheduling, MMCSS, timer resolution, big-core pinning). `false` makes every native entry point a no-op. |
| `--native_render_rt_priority` | int | `1` | Linux/Android: SCHED_FIFO priority attempted for the render thread (1 = lowest RT priority, range 1..99). |
| `--native_thread_nice` | int | `-5` | Linux/Android: nice value attempted for the render (fallback) and UI threads (range -20..19). |
| `--native_bigcore_threshold_permille` | int | `950` | Big-core detection threshold in permille of the fastest core's max frequency (500..1000). |
| `--native_mmcss_profile` | string | `Games` | Windows: MMCSS profile used when registering the render thread. |
| `--native_timer_period_ms` | int | `1` | Windows: timer resolution in ms requested via timeBeginPeriod (`0` disables). Fixes ~15.6 ms CV-wait granularity jitter. |
| `--verbose_logging` | bool | `false` | Enable verbose VLOG(1) output and mirror logs to stderr. Useful for debugging. |
| `--logtostderr` | bool | `false` | Write log messages to stderr instead of log files. |
| `--log_dir` | string | `./logs` | Directory where log files are stored. Created automatically if it does not exist. |

The GPU backend is **not** a runtime flag and not a NeoFlux concept: the
renderer is built entirely on tgfx, and the backend is tgfx's own
`TGFX_USE_*` CMake switch (exactly one per build, default `TGFX_USE_OPENGL=ON`).
See [Render backend](#render-backend-compile-time).

By default, logs are written to files in `./logs/` and no console window appears on Windows (`CMAKE_WIN32_EXECUTABLE`). To debug, pass `--logtostderr --verbose_logging`.

### Render backend (compile-time selection)

The GPU backend is tgfx's own concern. Pick one of tgfx's native switches at
configure time; `thirdparty/CMakeLists.txt` mirrors tgfx's own resolution
(fixed priority: VULKAN > D3D12 > METAL > OPENGL) and exports a single
`TGFX_USE_*=1` define so NeoFlux sources agree with what tgfx compiled:

```bash
cmake -S . -B build -G Ninja                                # default: OpenGL
cmake -S . -B build -G Ninja -DTGFX_USE_METAL=ON -DTGFX_USE_OPENGL=OFF   # Apple
cmake -S . -B build -G Ninja -DTGFX_USE_D3D12=ON -DTGFX_USE_OPENGL=OFF   # Windows
```

| tgfx backend | Status |
|---------|--------|
| `TGFX_USE_OPENGL` | ✅ Implemented (default): desktop WGL/GLX + Android EGL (`tgfx::EGLWindow`) + iOS EAGL path |
| `TGFX_USE_VULKAN` / `TGFX_USE_D3D12` / `TGFX_USE_METAL` | 🚧 Code paths exist (tgfx `Window` abstraction), not exercised in CI |

> Keep the default OpenGL unless you have a specific reason to experiment;
> `tgfx`'s CMake validates platform support (e.g. D3D12 requires Windows) and
> NeoFlux's `thirdparty/CMakeLists.txt` fails configure with an actionable
> message on unsupported combinations.

## Font System

NeoFlux uses a font manager that scans `thirdparty/fonts/` for TrueType (`.ttf`), OpenType (`.otf`), and TrueType Collection (`.ttc`) files at startup. Widgets reference fonts by filename stem (without extension):

```cpp
auto* text = new Text("Hello World");
text->SetFont("NotoSansSC-Regular");  // loads thirdparty/fonts/NotoSansSC-Regular.ttf
```

If no font is specified on a widget, the first discovered font is used as the default. Place your font files in `thirdparty/fonts/` and reference them by name — no build-time copying is required.

## Building Tests

Tests are disabled by default. Enable them with the `NEOFLUX_BUILD_TESTS` CMake option:

```bash
cmake -S . -B build -DNEOFLUX_BUILD_TESTS=ON
cmake --build build
cd build && ctest --output-on-failure
```

## Minimal Example

```cpp
#include <neoflux/neoflux.h>

using namespace neoflux;

std::shared_ptr<Widget> BuildHome(BuildContext& ctx) {
  auto root = std::make_shared<Container>();
  root->SetBackgroundColor({255, 255, 255, 255});

  auto text = std::make_shared<Text>("Hello NeoFlux!");
  text->SetFontSize(24.0F);

  auto button = std::make_shared<Button>("Click Me");
  button->SetOnPressed([]() { /* handle click */ });

  root->AddChild(text);
  root->AddChild(button);
  return root;
}

int main(int argc, char** argv) {
  RouteRegistry::Instance().RegisterRoute("/", BuildHome);

  Application app;
  app.Init(argc, argv, 800, 600, "NeoFlux");
  app.PushRoute("/");
  app.Run();
  return 0;
}
```

## Widget System

NeoFlux uses a Flutter-like widget model. Every UI element is a `Widget` that
can contain children. Layout is computed by the Taitank flexbox engine.

### Core Widgets

| Widget | Description |
|--------|-------------|
| `Widget` | Abstract base class. Override `Build()`, `OnMeasure()`, `Paint()`. |
| `Container` | Flexbox container with padding, margin, background color, border radius, flex direction. |
| `Text` | Single-line text with configurable font size, color, alignment. |
| `Button` | Clickable button with label, press callback, and pressed-state styling. |
| `ScrollView` | Scrollable viewport that clips and pans its content via mouse wheel / drag. |
| `Draggable` | Container that can be dragged with pointer input; paint-time translate so layout is unaffected. |
| `Expanded` | Container with `flex_grow` set; fills remaining space in a flex parent. |
| `SizedBox` | Container with explicit width/height; useful for fixed-size spacing. |
| `StatelessWidget` | Base for widgets that don't hold mutable state. |
| `StatefulWidget` | Base for widgets with mutable state; paired with `State<W>`. |

### Layout (Taitank Flexbox)

`Container` exposes flexbox properties that map directly to Taitank:

```cpp
auto col = std::make_shared<Container>();
col->SetFlexDirection(FlexDirection::kColumn)   // children stacked vertically
   ->SetJustifyContent(HAlign::kCenter)          // center on main axis
   ->SetAlignItems(VAlign::kCenter)              // center on cross axis
   ->SetPadding({.left = 16, .top = 16, .right = 16, .bottom = 16})
   ->SetBackgroundColor({.r = 245, .g = 245, .b = 250, .a = 255});
```

Leaf widgets (`Text`, `Button`) report their intrinsic size via `OnMeasure()`,
which Taitank calls during layout.

### Input Handling

Mouse/touch events flow from the platform bridge through the widget tree:

1. `GlfwBridge` receives GLFW mouse events (button, motion, scroll) and forwards them via callbacks.
2. `Application` performs a recursive `HitTest()` to find the deepest widget under the cursor. A hit-test cache avoids re-traversing the tree on every pointer-move event; the cache is invalidated whenever layout changes.
3. The hit widget's event handlers are called with local coordinates:
   - `OnPointerDown()` / `OnPointerUp()` — press and release
   - `OnPointerMove()` — cursor motion while hovering or dragging
   - `OnPointerEnter()` / `OnPointerExit()` — hover enter/leave transitions
4. `Button` overrides press/release to track state and invoke its `on_pressed` callback. `Draggable` overrides move to update its drag offset. `ScrollView` overrides move to support drag-to-scroll.

### Route Navigation

Widgets are registered with the `RouteRegistry` and pushed/popped onto a
navigation stack:

```cpp
RouteRegistry::Instance().RegisterRoute("/settings", BuildSettingsPage);
app.PushRoute("/settings");  // builds and displays the settings page
app.PopRoute();              // returns to the previous route
```

## Examples

### hello_neoflux

A complete demo showing stateful widgets, button callbacks, route navigation,
and flex layout. Run with:

```bash
./bin/hello_neoflux
```

### counter

A minimal counter app demonstrating `StatefulWidget` and `Button` callbacks.

```bash
./bin/counter
```

### flex_demo

A layout showcase demonstrating Taitank flex layout: row/column directions,
center justification, flex grow, and row reverse with colored boxes.

```bash
./bin/flex_demo
```

### font_demo

Demonstrates the font system: default font, explicit `SetFont()` selection,
multiple font sizes/colors, and CJK text rendering. Place fonts in
`thirdparty/fonts/` and reference them by name.

```bash
./bin/font_demo
```

### scroll_demo

Demonstrates `ScrollView`: a header bar plus a scrollable list of colored
items. Scroll with the mouse wheel or drag the content; content is clipped
to the viewport.

```bash
./bin/scroll_demo
```

### loading_demo

Demonstrates the widget state machine integrated with C++20 coroutines. A
"Start Loading" button transitions the widget to a loading state; a coroutine
animates a progress bar from 0% to 100% over ~2 seconds, yielding one frame
per step. On completion, the widget transitions to a success state.

```bash
./bin/loading_demo
```

### drag_demo

Demonstrates the `Draggable` widget with pointer events and the "state
machine as condition lock" pattern. A colored box can be dragged around; a
status label shows the current state (Idle/Hovering/Dragging) and offset.
A long-press coroutine is launched on pointer-down; if the pointer is
released before 500ms, the coroutine observes the state change and returns
silently. If held for 500ms+, a "[Long Press!]" indicator appears.

```bash
./bin/drag_demo
```

## Coroutines

NeoFlux supports C++20 coroutines for asynchronous work. Schedule a `Task<void>`
on the event loop; it resumes on the next frame when ready:

```cpp
#include <neoflux/core/task.h>

neoflux::Task<void> AnimateAsync() {
  for (int i = 0; i < 60; ++i) {
    co_await neoflux::Yield();  // resume next frame
    widget->SetOpacity(i / 60.0F);
  }
}

event_loop.Schedule(AnimateAsync());
```

### Sleep

Use `co_await Sleep(duration)` to suspend a coroutine for a wall-clock
duration. The event loop maintains a timer queue (`std::multimap` of
timepoints to coroutine handles) and resumes expired timers each frame:

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

### State Machine + Coroutine Pattern

Widgets carry a lightweight `WidgetState` (Idle, Hovering, Dragging, etc.).
State transitions are the "condition lock" for coroutines: a coroutine
launched on pointer-down checks the widget state after sleeping; if the
state has changed (e.g. pointer released), the coroutine returns silently.
No explicit cancellation is needed — the state machine gates execution.

## Project Structure

```
neoflux/
├── CMakeLists.txt          # Root build configuration
├── .clang-tidy             # clang-tidy rules
├── .clang-format           # Code style
├── cmake/                  # CMake modules
├── thirdparty/             # Third-party dependencies (FetchContent)
├── include/neoflux/        # Public headers
│   ├── core/               # Ring queue, types, utilities
│   ├── widget/             # Widget system (Widget, Container, Text, Button)
│   ├── app/                # Application, EventLoop
│   └── render/             # Render layer, commands, tgfx, GLFW
├── src/                    # Implementation
├── tests/                  # GTest unit tests
├── examples/               # Example applications
└── docs/                   # Documentation
```
## Mobile Rendering

Mobile builds do not use GLFW. tgfx renders directly into the platform-provided
surface, and the GPU context is owned entirely by tgfx:

- **Android**: the app shell hands over an `ANativeWindow*` as
  `platform_surface`; `TgfxRenderer` wraps it in a `tgfx::EGLWindow`, which
  creates the EGL display/context/surface and presents on `context->submit()`.
- **iOS**: the equivalent `tgfx::EAGLWindow::MakeFrom(CAEAGLLayer*)` path
  requires the ObjC++ app shell (not yet in this repository).

Touch input is dispatched by `MobileBridge` (the mobile implementation of
`PlatformBridge`): the shell calls its `DispatchTouchEvent()` from the JNI/UI
thread, and `Application::Init` wires the callback into
`DispatchPointerEvent()` — the same hit-test pipeline as the desktop mouse
path, with `MouseButton::kTouch`.

```cpp
// Mobile initialization example (from the app shell):
app.Init(argc, argv, width, height, "NeoFlux", platform_surface);
```

On desktop, pass `nullptr` for `platform_surface` and the framework creates
the GLFW window itself.
