# NeoFlux++ — 轻量 · 极速 · 声明式 C++20 UI 框架

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/compiler_support)
[![License](https://img.shields.io/badge/license-GPLv3-green.svg)](LICENSE)

NeoFlux++ 是一个面向未来桌面与移动应用的轻量级 C++20 UI 框架。它融合了声明式组件模型、布局与渲染分层、线程友好的基础设施和现代 CMake 构建流程，目标是在保持高性能与低复杂度的同时，提供接近 Flutter 风格的组合式开发体验。

## ✨ 项目定位

- **声明式 UI**：通过组合小组件构建复杂界面。
- **模块化架构**：核心层、引擎层、应用层和测试层分离清晰。
- **高性能基础设施**：内置线程池、信号量、环形缓冲区和轻量型布局/渲染桥接。
- **现代构建**：基于 CMake 3.20+，第三方依赖通过子模块与本地构建目录管理。

## 🧱 项目结构

- app：应用入口与示例界面。
- framework：核心框架、布局/渲染引擎、基础 widget 类型。
- tests：GoogleTest 测试用例。
- thirdparty：gflags、glog、googletest、Skia、Taitank 等依赖子模块。

## 🚀 快速开始

### 1. 初始化第三方依赖

```bash
./scripts/setup_thirdparty.sh
```

### 2. 构建项目

```bash
./scripts/build.sh
```

### 3. 运行示例应用

```bash
./build/app/neoflux_app
```

### 4. 运行测试

```bash
ctest --test-dir build --output-on-failure
```

## 🧪 示例：构建一个简单界面

下面的示例展示了如何组合一个容器、标题文本和按钮：

```cpp
#include <neoflux/widgets/ButtonWidget.h>
#include <neoflux/widgets/ContainerWidget.h>
#include <neoflux/widgets/TextWidget.h>

#include <memory>

std::shared_ptr<neoflux::core::Widget> buildDemoWidget() {
  auto root = std::make_shared<neoflux::widgets::ContainerWidget>(
      neoflux::widgets::ContainerWidget::Direction::Column);
  auto title = std::make_shared<neoflux::widgets::TextWidget>("Welcome to NeoFlux");
  auto button = std::make_shared<neoflux::widgets::ButtonWidget>("Click Me");
  root->addChild(title);
  root->addChild(button);
  return root;
}
```

## 🛠️ 构建要求

- CMake 3.20+
- C++20 编译器（GCC/Clang/MSVC）
- 可用的 POSIX 线程环境

## 📌 当前状态

当前仓库已经具备可构建的骨架：

- 可构建应用目标 `neoflux_app`
- 可构建测试目标 `neoflux_test`
- 支持通过 `neoflux` 头文件路径访问框架 API

后续会继续完善布局/渲染引擎、Skia 集成和更完整的 UI 组件系统。
