# 快速开始

## 环境要求

- CMake 3.20+
- 支持 C++20 的编译器（GCC 11+ / Clang 14+ / MSVC 2022）
- Git（用于 FetchContent 下载依赖）

## 构建

```bash
mkdir build
cd build
cmake .. -G Ninja
cmake --build .
```

Windows MinGW 示例：

```bash
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make -j4
```

## 运行示例

构建完成后，可执行文件在 `bin/` 目录下：

```bash
./bin/hello_neoflux   # 完整演示
./bin/counter         # 计数器
./bin/flex_demo       # flex 布局
./bin/font_demo       # 字体系统
./bin/scroll_demo     # 滚动视图
./bin/loading_demo    # 状态机 + 协程动画
./bin/drag_demo       # 可拖拽 Widget
```

## 编译测试

测试默认禁用，通过 CMake 选项启用：

```bash
cmake -S . -B build -DNEOFLUX_BUILD_TESTS=ON
cmake --build build
cd build && ctest --output-on-failure
```

## 最小示例

```cpp
#include <neoflux/neoflux.h>

using namespace neoflux;

std::shared_ptr<Widget> BuildHome(BuildContext& ctx) {
  auto root = std::make_shared<Container>();
  root->SetBackgroundColor({.r = 255, .g = 255, .b = 255, .a = 255});

  auto text = std::make_shared<Text>("Hello NeoFlux!");
  text->SetFontSize(24.0F);

  auto button = std::make_shared<Button>("Click Me");
  button->SetOnPressed([]() { /* 处理点击 */ });

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

## 下一步

- [架构概览](./architecture)
- [Widget 系统](./widgets)
- [Flex 布局](./layout)
