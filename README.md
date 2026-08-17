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
- Cross-platform: Windows, Linux, macOS (desktop), Android/iOS (mobile)
- C++20 standard with modern features (std::string_view, ranges, concepts)
- Google C++ style guide compliance
- clang-tidy static analysis
- GLog logging + GFlags command-line parsing
- GTest unit testing
- CMake build system with FetchContent dependency management

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

Examples are **off by default**. Enable them at configure time:

```bash
cmake -S . -B build -DNEOFLUX_BUILD_EXAMPLES=ON
cmake --build build
./build/bin/hello_neoflux
```

## Configuration (gflags)

NeoFlux uses gflags for runtime configuration. All flags are optional.

| Flag | Type | Default | Description |
|------|------|---------|-------------|
| `--target_fps` | int | `60` | Target frame rate for the application event loop and render pacing. |
| `--render_queue_capacity` | int | `2048` | Capacity of the SPSC lock-free ring queue between the application and render layers. Rounded up to the next power of two automatically. |
| `--verbose_logging` | bool | `false` | Enable verbose VLOG(1) output and mirror logs to stderr. Useful for debugging. |
| `--logtostderr` | bool | `false` | Write log messages to stderr instead of log files. |
| `--log_dir` | string | `./logs` | Directory where log files are stored. Created automatically if it does not exist. |
| `--render_backend` | string | `vulkan` | Render backend selection: `vulkan`, `gl`, or `cpu`. Vulkan/CPU fall back to OpenGL with a warning when not yet implemented. |

By default, logs are written to files in `./logs/` and no console window appears on Windows (`CMAKE_WIN32_EXECUTABLE`). To debug, pass `--logtostderr --verbose_logging`.

## Font System

NeoFlux uses a font manager that scans `thirdparty/fonts/` for TrueType (`.ttf`), OpenType (`.otf`), and TrueType Collection (`.ttc`) files at startup. Widgets reference fonts by filename stem (without extension):

```cpp
auto* text = new Text("Hello World");
text->SetFont("NotoSansSC-Regular");  // loads thirdparty/fonts/NotoSansSC-Regular.ttf
```

If no font is specified on a widget, the first discovered font is used as the default. Place your font files in `thirdparty/fonts/` and reference them by name — no build-time copying is required.

> **Warning:** If `thirdparty/fonts/` is empty, text rendering will fail or
> show garbled output. Always include at least one font file (e.g. a CJK font
> for Chinese text) before shipping.

## Building Tests

Tests and example are disabled by default. Enable them with the `NEOFLUX_BUILD_TESTS` CMake option:

```bash
cmake -S . -B build -DNEOFLUX_BUILD_TESTS=ON -NEOFLUX_BUILD_EXAMPLES=ON
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

> **Tip:** Even with a single route, you must register it and call
> `PushRoute` — `Init` does not display anything automatically.

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

> **Warning:** Coroutines that capture widget pointers must use
> `std::weak_ptr` and re-lock after each `co_await`. A widget can be
> destroyed while a coroutine is suspended on `Sleep` or `Yield`; accessing
> a raw pointer after resumption causes use-after-free.

## Mobile Rendering

On mobile, NeoFlux does not use GLFW. Instead, tgfx renders directly into a
platform-provided surface:

- **Android**: pass an `ANativeWindow*` as `platform_surface`
- **iOS**: pass a `CAMetalLayer*` or `CAEAGLLayer*` as `platform_surface`

```cpp
// Mobile initialization example
app.Init(argc, argv, width, height, "NeoFlux", platform_surface);
```

On desktop, pass `nullptr` for `platform_surface` and the framework creates a
GLFW window automatically.

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