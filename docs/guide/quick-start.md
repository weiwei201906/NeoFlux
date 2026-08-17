# Quick Start

This guide walks through creating a minimal NeoFlux application from scratch.

## 1. Create the Main File

```cpp
#include <neoflux/app/application.h>
#include <neoflux/widget/container.h>
#include <neoflux/widget/text.h>
#include <neoflux/widget/route_registry.h>

using namespace neoflux;

// Build function: returns the root widget for a route.
std::shared_ptr<Widget> BuildHomePage(BuildContext& /*context*/) {
  auto root = std::make_shared<Container>();
  root->SetFlexDirection(FlexDirection::kColumn)
      .SetBackgroundColor({.r = 245, .g = 245, .b = 245, .a = 255})
      .SetJustifyContent(HAlign::kCenter)
      .SetAlignItems(VAlign::kCenter);

  auto title = std::make_shared<Text>("Hello NeoFlux");
  title->SetFontSize(24.0F)
      .SetTextColor({.r = 33, .g = 33, .b = 33, .a = 255});
  root->AddChild(title);

  return root;
}

int main(int argc, char** argv) {
  // Register the route.
  RouteRegistry::Instance().RegisterRoute("/", BuildHomePage);

  // Create and initialize the application.
  Application app;
  if (!app.Init(argc, argv, 400, 300, "My App")) {
    return 1;
  }

  // Push the initial route and run the event loop.
  app.PushRoute("/");
  app.Run();
  return 0;
}
```

## 2. CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_app LANGUAGES CXX)

# Add NeoFlux as a subdirectory or find_package.
add_subdirectory(neoflux)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE neoflux)
```

## 3. Build and Run

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
./my_app
```

## Key Concepts in This Example

1. **Route builder function** — `BuildHomePage` returns a widget tree. It is
   called when the route is pushed.
2. **Container** — the base layout widget. Configure flex direction, alignment,
   padding, and background color.
3. **Text** — a leaf widget that renders text. Set font size and color.
4. **RouteRegistry** — maps route names to builder functions.
5. **Application** — owns the window, event loop, render layer, and navigation
   stack.

## Next Steps

- Learn about the [widget system](./widgets)
- Explore [flex layout](./layout)
- Handle [user input](./input)
- Add [navigation](./routing) between pages
