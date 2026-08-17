# Quick Start

This guide walks through creating your first NeoFlux application from scratch.

## 1. Project Structure

Create a new directory for your project:

```
my_app/
├── CMakeLists.txt
├── main.cpp
└── neoflux/          # NeoFlux source (git submodule or copy)
```

## 2. Get NeoFlux

### Option A: Git Submodule (recommended)

```bash
git init
git submodule add https://github.com/weiwei201906/NeoFlux.git neoflux
```

### Option B: FetchContent (no submodule)

Add this to your `CMakeLists.txt` (see below) — NeoFlux is downloaded
automatically at configure time.

## 3. main.cpp

```cpp
#include <neoflux/app/application.h>
#include <neoflux/widget/container.h>
#include <neoflux/widget/text.h>
#include <neoflux/widget/button.h>
#include <neoflux/widget/route_registry.h>

using namespace neoflux;

// Route builder: returns the root widget tree for "/"
std::shared_ptr<Widget> BuildHomePage(BuildContext& /*ctx*/) {
  auto root = std::make_shared<Container>();
  root->SetFlexDirection(FlexDirection::kColumn)
      .SetBackgroundColor({.r = 245, .g = 245, .b = 245, .a = 255})
      .SetJustifyContent(HAlign::kCenter)
      .SetAlignItems(VAlign::kCenter)
      .SetPadding({.left = 24, .top = 24, .right = 24, .bottom = 24});

  auto title = std::make_shared<Text>("Hello NeoFlux");
  title->SetFontSize(28.0F)
      .SetTextColor({.r = 33, .g = 33, .b = 33, .a = 255});
  root->AddChild(title);

  auto button = std::make_shared<Button>("Click Me");
  button->SetOnPressed([]() {
    LOG(INFO) << "Button pressed!";
  });
  root->AddChild(button);

  return root;
}

int main(int argc, char** argv) {
  // Register routes before initializing the app.
  RouteRegistry::Instance().RegisterRoute("/", BuildHomePage);

  Application app;
  if (!app.Init(argc, argv, 480, 360, "My First NeoFlux App")) {
    return 1;
  }

  // Push the initial route and run the event loop (blocks until window close).
  app.PushRoute("/");
  app.Run();
  return 0;
}
```

## 4. CMakeLists.txt

### With submodule

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(neoflux)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE neoflux)
```

### With FetchContent

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  neoflux
  GIT_REPOSITORY https://github.com/weiwei201906/NeoFlux.git
  GIT_TAG main
)
FetchContent_MakeAvailable(neoflux)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE neoflux)
```

## 5. Build and Run

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
./my_app
```

You should see a window with "Hello NeoFlux" text and a clickable button.

## Key Concepts

| Concept | Description |
|---------|-------------|
| **Route builder** | A function that returns a widget tree. Called when the route is pushed. |
| **Container** | Base layout widget. Flex direction, alignment, padding, background. |
| **Text** | Leaf widget that renders UTF-8 text. |
| **Button** | Clickable widget with `SetOnPressed()` callback. |
| **RouteRegistry** | Maps route names to builder functions. |
| **Application** | Owns window, event loop, render layer, and navigation stack. |

## Next Steps

- Learn the [widget system](./widgets)
- Explore [flex layout](./layout)
- Handle [user input](./input)
- Add [navigation](./routing) between pages
- Use [coroutines](./coroutines) for animations and timed work
