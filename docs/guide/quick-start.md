# Quick Start

This guide walks you from a fresh clone to a running NeoFlux window on Windows
with the MSVC toolchain and the Ninja build generator.

## Prerequisites

- **CMake 3.20 or newer**
- **MSVC 2022** (Visual Studio 17) with the "Desktop development with C++" workload
- **Ninja** build tool (bundled with Visual Studio, or installed separately)
- A C++20-compatible compiler (MSVC 19.3x+)

Third-party dependencies are vendored under `thirdparty/` as git submodules
(`gflags`, `glog`, `Taitank`, `GLFW`, `FreeType`, `tgfx`) -- you do not need
to install them by hand. On a fresh clone, run:

```powershell
git submodule update --init --recursive
```

::: tip OpenGL on Windows
The desktop build creates its context through GLFW/WGL, so any modern GPU
driver that exposes OpenGL 2.1+ works.
:::

## Build the repo (MSVC + Ninja)

Open an **x64 Native Tools Command Prompt for VS 2022** (or run
`vcvars64.bat` from a regular terminal) so that `cl.exe`, `link.exe` and
`ninja` are on `PATH`, then configure and build from the repository root:

```powershell
# Configure (out-of-source build)
cmake -S . -B build -G Ninja

# Build the static library + all examples
cmake --build build
```

Build artifacts land under `build\bin\`:

| Path                          | Content                      |
|-------------------------------|------------------------------|
| `build\bin\neoflux_app.exe`   | The scaffolded quick-start app (home + media routes) |
| `build\bin\media_player_demo.exe` | Standalone media player demo |
| `build\bin\counter.exe`       | Counter demo                 |
| `build\bin\flex_demo.exe`     | Flex layout showcase         |
| `build\bin\font_demo.exe`     | Font rendering demo          |
| `build\bin\scroll_demo.exe`   | ScrollView demo              |
| `build\bin\loading_demo.exe`  | Coroutine state-machine demo |
| `build\bin\drag_demo.exe`     | Draggable + long-press demo  |
| `build\lib\`                  | `neoflux` static library     |

To build with unit tests enabled instead:

```powershell
cmake -S . -B build -G Ninja -DNEOFLUX_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

::: warning GUI subsystem, no console
Examples are built with `CMAKE_WIN32_EXECUTABLE ON`, so on Windows the
launcher does **not** open a console window. By default logs are written to
files under `./logs/`, not to the terminal. Use the logging flags below when
you want to see output.
:::

## Run the example

From the repository root (so the relative `assets/fonts` and `./logs`
paths resolve):

```powershell
.\build\bin\neoflux_app.exe --logtostderr
```

A window titled *NeoFlux* opens. You will see a home screen with buttons to
navigate to different demos. Click **Media Player** to open the video player.

## Runtime flags (gflags)

All flags are optional. Pass them on the command line after the executable:

```powershell
.\build\bin\hello_neoflux.exe --render_backend=gl --target_fps=120 --logtostderr --verbose_logging
```

| Flag | Example value | What it does |
|------|---------------|--------------|
| `--render_backend` | `gl` | Selects the render backend. Defaults to `gl` (the only backend available in this build); `vulkan`, `cpu`, and unknown values are a hard startup error with no silent fallback. |
| `--target_fps` | `120` | Caps the application event-loop frame rate. |
| `--render_queue_capacity` | `4096` | Size of the SPSC render-command ring queue (rounded up to a power of two). |
| `--verbose_logging` | (present) | Enables `VLOG(1)` debug output. |
| `--logtostderr` | (present) | Sends all logs to stderr instead of files. |
| `--log_dir=PATH` | `--log_dir=./logs` | Directory for `.log` files (default `./logs`). |

::: tip Fast debugging loop
For day-to-day development, run with stderr logging and verbose output:

```powershell
.\build\bin\hello_neoflux.exe --logtostderr --verbose_logging
```

See the [Configuration](./configuration) page for the complete flag reference.
:::

## A minimal application

Every NeoFlux program follows the same shape: register routes, create an
`Application`, push the initial route, then run the blocking event loop.

```cpp
#include <neoflux/app/application.h>
#include <neoflux/widgets/button.h>
#include <neoflux/widgets/container.h>
#include <neoflux/widgets/route_registry.h>
#include <neoflux/widgets/text.h>
#include <neoflux/widgets/widget.h>

using namespace neoflux;

std::shared_ptr<Widget> BuildHomePage(BuildContext& /*ctx*/) {
  auto root = std::make_shared<Container>();
  root->SetFlexDirection(FlexDirection::kColumn)
      .SetJustifyContent(HAlign::kCenter)
      .SetAlignItems(VAlign::kCenter)
      .SetBackgroundColor({.r = 245, .g = 245, .b = 245, .a = 255})
      .SetPadding({.left = 24, .top = 24, .right = 24, .bottom = 24});

  auto title = std::make_shared<Text>("Hello NeoFlux");
  title->SetFontSize(28.0F).SetTextColor({.r = 33, .g = 33, .b = 33, .a = 255});
  root->AddChild(title);

  auto button = std::make_shared<Button>("Click Me");
  button->SetOnPressed([]() { /* handle click */ });
  root->AddChild(button);
  return root;
}

int main(int argc, char** argv) {
  RouteRegistry::Instance().RegisterRoute("/", BuildHomePage);

  Application app;
  if (!app.Init(argc, argv, 480, 360, "My First NeoFlux App")) {
    return 1;
  }
  app.PushRoute("/");
  app.Run();
  return 0;
}
```

Link against the `neoflux` static target (plus `glog::glog` and `gflags` if
you use logging/flags directly), and you are set.

## Next steps

- Read [Architecture](./architecture) to understand the two-layer design and
  how render commands cross threads.
- Walk through the [Examples](../examples/hello) to see real widget code.
- Explore the [Widgets guide](./widgets) for the full widget set.
