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

```bash
./bin/hello_neoflux
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

By default, logs are written to files in `./logs/` and no console window appears on Windows (MinGW `-mwindows`). To debug, pass `--logtostderr --verbose_logging`.

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
| `Container` | Flexbox container with padding, margin, background color, flex direction. |
| `Text` | Single-line text with configurable font size, color, alignment. |
| `Button` | Clickable button with label, press callback, and pressed-state styling. |
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

1. `GlfwBridge` receives GLFW mouse events and forwards them via `InputEventCallback`.
2. `Application` performs a recursive `HitTest()` to find the deepest widget under the cursor.
3. The hit widget's `OnPointerDown()` / `OnPointerUp()` is called with local coordinates.
4. `Button` overrides these to track press state and invoke its `on_pressed` callback.

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