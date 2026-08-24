# 快速开始

本指南从零开始创建你的第一个 NeoFlux 应用。

## 1. 项目结构

为项目创建新目录。NeoFlux 应放在 `thirdparty/` 下，将依赖与源码隔离：

```
my_app/
├── CMakeLists.txt
├── main.cpp
└── thirdparty/
    └── neoflux/      # NeoFlux 源码（git submodule 或拷贝）
```

## 2. 获取 NeoFlux

### 方式 A：Git Submodule（推荐）

```bash
git init
git submodule add https://github.com/weiwei201906/NeoFlux.git thirdparty/neoflux
```

### 方式 B：FetchContent（无需 submodule）

在 `CMakeLists.txt` 中添加（见下文）——NeoFlux 在配置时自动下载到 `thirdparty/`。

## 3. main.cpp

```cpp
#include <neoflux/app/application.h>
#include <neoflux/widget/container.h>
#include <neoflux/widget/text.h>
#include <neoflux/widget/button.h>
#include <neoflux/widget/route_registry.h>

using namespace neoflux;

// 路由构建函数：返回 "/" 路由的根 widget 树
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
  // 在初始化应用前注册路由。
  RouteRegistry::Instance().RegisterRoute("/", BuildHomePage);

  Application app;
  // 在 Init() 之前配置字体目录。将 .ttf/.otf 文件放入 fonts/
  // （或你自定义的目录）。详见字体系统文档。
  app.SetFontDir("fonts");
  if (!app.Init(argc, argv, 480, 360, "My First NeoFlux App")) {
    return 1;
  }

  // 压入初始路由并运行事件循环（阻塞直到窗口关闭）。
  app.PushRoute("/");
  app.Run();
  return 0;
}
```

:::tip
哪怕只有一个路由，也必须先 `RegisterRoute` 再 `PushRoute`。`Init` 只创建窗口——在推送路由之前不会显示任何 Widget。
:::

:::warning
文本 Widget 需要字体文件。运行前请在配置的字体目录（默认 `fonts/`）中至少放入一个 `.ttf`/`.otf` 字体。没有字体时，所有文本会显示为乱码或空白。
:::

## 4. CMakeLists.txt

### 使用 submodule

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# NeoFlux 放在 thirdparty/ 下，隔离依赖。
add_subdirectory(thirdparty/neoflux)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE neoflux)

# 每次构建时将 fonts/ 目录拷贝到可执行文件同级目录。
# 将你的 .ttf/.otf 文件放入 ${CMAKE_SOURCE_DIR}/fonts/
add_custom_command(TARGET my_app POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/fonts $<TARGET_FILE_DIR:my_app>/fonts
)
```

> **注意：** NeoFlux 的示例和测试**默认关闭**。如需构建，在配置时传入
> `-DNEOFLUX_BUILD_EXAMPLES=ON -DNEOFLUX_BUILD_TESTS=ON`。
>
> **字体：** 运行示例需要 `thirdparty/fonts/` 目录下有字体文件（示例显式调用 SetFontDir("thirdparty/fonts")）。没有字体时，文本会显示为乱码或空白。

### 使用 FetchContent

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
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/neoflux
)
FetchContent_MakeAvailable(neoflux)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE neoflux)
```

## 5. 构建并运行

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
./my_app
```

你应该看到一个窗口，显示 "Hello NeoFlux" 文本和一个可点击的按钮。

## 核心概念

| 概念 | 说明 |
|------|------|
| **路由构建函数** | 返回 widget 树的函数，路由被压入时调用。 |
| **Container** | 基础布局组件。Flex 方向、对齐、内边距、背景色。 |
| **Text** | 渲染 UTF-8 文本的叶子组件。 |
| **Button** | 可点击组件，通过 `SetOnPressed()` 设置回调。 |
| **RouteRegistry** | 将路由名称映射到构建函数。 |
| **Application** | 拥有窗口、事件循环、渲染层和导航栈。 |

## 下一步

- 学习 [Widget 系统](./widgets)
- 探索 [Flex 布局](./layout)
- 处理 [用户输入](./input)
- 添加页面间 [路由导航](./routing)
- 使用 [协程](./coroutines) 实现动画和定时工作
