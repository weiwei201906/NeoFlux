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
| `--target_fps` | int | `60` | Target frame rate for the render loop. |
| `--render_queue_capacity` | int | `2048` | Capacity of the SPSC lock-free ring queue between the application and render layers. Must be a power of two (rounded up automatically). |
| `--verbose_logging` | bool | `false` | Enable verbose VLOG(1) output and mirror logs to stderr. Useful for debugging. |
| `--logtostderr` | bool | `false` | Write log messages to stderr instead of log files. (Built-in glog flag.) |
| `--log_dir` | string | `./logs` | Directory where log files are stored. Created automatically if it does not exist. (Built-in glog flag.) |

By default, logs are written to files in `./logs/` and no console window appears on Windows. To debug, pass `--logtostderr --verbose_logging`.

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