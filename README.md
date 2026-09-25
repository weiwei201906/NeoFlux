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
- CMake build system with Git Submodule dependency management

## Quick Start

### Prerequisites

- CMake 3.20+
- C++20 compatible compiler (GCC 11+, Clang 14+, MSVC 2022+)
- Ninja or Make / Visual Studio
- Git (for submodule initialization)

### Clone

```bash
git clone https://github.com/weiwei201906/NeoFlux.git
cd NeoFlux
git submodule update --init --recursive
```

The `git submodule` command fetches all third-party dependencies (glog,
gflags, glfw, taitank, freetype, gtest, tgfx, mpv) under `thirdparty/`.
`thirdparty/mpv` is the libmpv media backend source; the prebuilt bundle
(`thirdparty/mpv-bundle/`, gitignored) is auto-detected by CMake for
out-of-the-box Windows media playback.

### Build

```bash
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Run the Quick-Start App

```bash
./bin/neoflux_quickstart
```

A window with "NeoFlux Quick Start" text should appear. The quick-start source
lives in `src/main.cpp` — replace it with your own UI.

### Run Tests

```bash
cmake -S .. -B . -DNEOFLUX_BUILD_TESTS=ON
cmake --build .
ctest --output-on-failure
```

## Configuration (gflags)

NeoFlux uses gflags for runtime configuration. All flags are optional.

| Flag | Type | Default | Description |
|------|------|---------|-------------|
| `--target_fps` | int | `60` | Target frame rate for the application event loop and render pacing. |
| `--render_queue_capacity` | int | `2048` | Capacity of the SPSC lock-free ring queue between the application and render layers. Rounded up to the next power of two automatically. |
| `--render_backend` | string | `vulkan` | Render backend selection: `vulkan`, `gl`, or `cpu`. Vulkan/CPU fall back to OpenGL with a warning when not yet implemented. |
| `--verbose_logging` | bool | `false` | Enable verbose VLOG(1) output and mirror logs to stderr. Useful for debugging. |
| `--logtostderr` | bool | `false` | Write log messages to stderr instead of log files. |
| `--log_dir` | string | `./logs` | Directory where log files are stored. Created automatically if it does not exist. |

By default, logs are written to files in `./logs/` and no console window
appears on Windows (`CMAKE_WIN32_EXECUTABLE`). To debug, pass
`--logtostderr --verbose_logging`.

## Font System

NeoFlux scans a configurable directory for TrueType (`.ttf`), OpenType
(`.otf`), and TrueType Collection (`.ttc`) files at startup. The default
directory is `assets/fonts/`. Font files are **not committed to git** — place
your own fonts there (or download Noto Sans SC; see
[assets/fonts/README.md](assets/fonts/README.md)).

**Configure the font directory before `Init()`:**

```cpp
Application app;
app.SetFontDir("./assets/fonts/");  // optional: override default "assets/fonts"
app.Init(argc, argv, 800, 600, "NeoFlux");
```

Widgets reference fonts by filename stem (without extension):

```cpp
auto text = std::make_shared<Text>("Hello World");
text->SetFont("NotoSansSC-Regular");  // loads <font_dir>/NotoSansSC-Regular.ttf
```

If no font is specified on a widget, the first discovered font is used as the
default. Relative paths are searched from the working directory with upward
fallback (`../`, `../../`) for build subdirectories.

> **Warning:** If the configured font directory is empty, text rendering will
> fail or show garbled output. Always include at least one font file (e.g. a
> CJK font for Chinese text). Call `SetFontDir()` before `Init()` to specify a
> custom directory.

### CMake: Auto-Copy Fonts at Build Time

NeoFlux's CMake copies fonts from the repository `assets/fonts/` directory to
`<output>/assets/fonts/` in a POST_BUILD step. In your own project:

```cmake
# CMakeLists.txt
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE neoflux)

# Copy assets/fonts/ next to the executable on every build
add_custom_command(TARGET my_app POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/assets/fonts $<TARGET_FILE_DIR:my_app>/assets/fonts
)
```

Then configure in code:

```cpp
Application app;
app.SetFontDir("./assets/fonts/");  // matches the copied assets/fonts/ folder
app.Init(argc, argv, 800, 600, "My App");
```

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `NEOFLUX_BUILD_TESTS` | `OFF` | Build unit tests (gtest). |
| `NEOFLUX_ENABLE_CLANG_TIDY` | `OFF` | Run clang-tidy as a build step. |
| `NEOFLUX_USE_TGFX` | `OFF` | Use tgfx rendering backend (requires MSVC on Windows). |
| `NEOFLUX_USE_MPV` | `ON` | Enable the libmpv media backend (desktop). At configure time the build probes `thirdparty/mpv-bundle/` then `thirdparty/mpv/` then the system libmpv; when none is found the media widget compiles as disabled with a warning. |

## Building Tests

Tests are disabled by default. Enable them with the `NEOFLUX_BUILD_TESTS`
CMake option:

```bash
cmake -S . -B build -DNEOFLUX_BUILD_TESTS=ON
cmake --build build -j 16
cd build && ctest --output-on-failure
```

## Build Output Structure

```
build/bin/
├── neoflux_quickstart.exe   (quick-start app, ~1.2MB)
├── glog.dll                 (auto-copied, same dir as exe)
├── libmpv-2.dll             (auto-copied, same dir as exe, if enabled)
└── assets/
    └── fonts/
        └── NotoSansCJKsc-Regular.otf  (auto-copied from assets/fonts/)
```

- **Windows**: DLLs are placed next to the executable (standard Windows
  deployment). No launcher scripts or PATH setup needed — double-click the exe
  to run.
- **Linux/macOS**: Shared libraries are found relative to the executable via
  RPATH.
- **Fonts**: Place your own `.ttf`/`.otf`/`.ttc` files in `assets/fonts/`
  before building. Fonts are gitignored.

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
  app.SetFontDir("./assets/fonts/");
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
| `TextField` | Single-line editable text input with cursor navigation, placeholder, UTF-8 support, and focus management. |
| `MediaWidget` | Embedded media playback backed by libmpv (desktop); video frames are decoded into an OpenGL texture and composited. |
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

## Media Playback (libmpv)

The desktop media widget is backed by [libmpv](https://mpv.io/) (GPL-3.0, matching
NeoFlux's license). Video frames are decoded via the mpv render API into an
OpenGL texture and composited by the render layer.

- `thirdparty/mpv/` is the mpv **source submodule** (fetch with
  `git submodule update --init thirdparty/mpv`) for building libmpv yourself.
- Out of the box on Windows: drop a prebuilt bundle into
  `thirdparty/mpv-bundle/` (`include/mpv/`, `libmpv.dll.a`, `libmpv-2.dll`);
  CMake detects it automatically, links it, and copies `libmpv-2.dll` next to
  your executable.
- Linux/macOS: install system libmpv (`apt install libmpv-dev` /
  `brew install mpv`); CMake discovers it via `find_package(mpv)` /
  `pkg-config`.
- When no libmpv is available, `MediaWidget` compiles as disabled with a
  warning; the rest of the framework is unaffected.

The media player unit test `neoflux/tests/mpv_media_player_test.cpp` (enabled
with `NEOFLUX_BUILD_TESTS=ON`) exercises the full pipeline against the bundled
2-second 320x240 H.264 clip `tests/data/sample.mp4` (~120 KB, committed):
load -> play -> first frame -> pause -> stop -> seek -> state callback.
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
NeoFlux/
├── CMakeLists.txt           # Host quick-start build (adds thirdparty + neoflux)
├── .clang-tidy              # clang-tidy rules
├── .clang-format            # Code style
├── assets/
│   └── fonts/               # Font files (gitignored; place your own fonts here)
├── neoflux/                 # Self-contained framework (also usable as submodule)
│   ├── CMakeLists.txt       # Framework library build
│   ├── include/neoflux/     # Public headers
│   │   ├── core/            # Ring queue, task, types, utilities
│   │   ├── widget/          # Widget system (Widget, Container, Text, Button)
│   │   ├── app/             # Application, EventLoop
│   │   ├── render/          # Render layer, commands, tgfx facade
│   │   └── native/          # Platform bridges (GLFW, mobile, GL renderer)
│   ├── src/                 # Implementation
│   ├── tests/               # GTest unit tests
│   └── cmake/               # CMake modules (CompilerFlags, android, ios)
├── src/                     # Quick-start host project (your application)
│   ├── main.cpp             # Entry: RegisterRoutes -> Init -> PushRoute("/") -> Run
│   ├── router/              # Route registry (index.h/.cpp; register every route here)
│   └── views/               # One view per route (home/counter/about)
├── thirdparty/              # Git submodules (glog, gflags, glfw, taitank, tgfx, mpv, ...)
├── docs/                    # VitePress documentation (bilingual)
├── README.md
└── README-zh.md
```

## Documentation

Full bilingual documentation (VitePress) is under `docs/`:

- [Guide (English)](docs/guide/introduction.md)
- [Guide (中文)](docs/zh/guide/introduction.md)

## Contributing

See [docs/guide/contributing.md](docs/guide/contributing.md). Requirements:

- C++20, Google C++ style guide, pure ASCII source
- clang-tidy zero-warning pass before every PR
- RAII + smart pointers; no raw owning pointers
- Unit tests (gtest) for new functionality
- Local reproduction of every change before submission
