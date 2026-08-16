# NeoFlux
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/compiler_support)
[![License](https://img.shields.io/badge/license-GPLv3-green.svg)](LICENSE)

一个跨平台 C++20 UI 框架，采用类 Flutter 的 Widget 开发模型。

> 本项目采用 **GNU Affero General Public License v3.0 (AGPL-3.0)** 开源协议。详见 [LICENSE](LICENSE) 文件。

## 架构

NeoFlux 采用两层架构，层间通过无锁环形队列通信：

```
+---------------------------+         +---------------------------+
|   Application Layer       |  SPSC   |   Render Layer            |
|   (UI 线程)               |  Ring   |   (渲染线程)              |
|                           |  Queue  |                           |
|  - Widget Tree            | +-----> |  - tgfx Renderer          |
|  - Taitank Layout Engine  |         |  - GLFW Bridge (桌面端)  |
|  - Event Loop             |         |  - Command Execution      |
|  - Route Navigation       |         |                           |
+---------------------------+         +---------------------------+
```

- **Application 层**：运行业务逻辑，构建 Widget 树，通过 Taitank 计算布局，记录渲染命令。
- **Render 层**：从 SPSC 环形队列消费命令，使用 tgfx 执行绘制。移动端 tgfx 直接渲染到平台 Surface（ANativeWindow / CAMetalLayer），桌面端通过 GLFW 创建窗口 + OpenGL 上下文。
- **SPSC 环形队列**：无锁单生产者单消费者有界环形缓冲区，缓存行对齐避免 false sharing，无需互斥锁。

## 特性

- 类 Flutter Widget 系统（StatelessWidget / StatefulWidget / State）
- 基于路由的 Widget 注册与导航
- 跨平台：Windows / Linux / macOS（桌面），Android / iOS（移动）
- C++20 标准，使用 `std::string_view`、`std::construct_at`、`std::atomic` 内存序等现代特性
- 遵循 Google C++ 编码规范
- clang-tidy 静态分析
- GLog 日志 + GFlags 命令行参数解析
- GTest 单元测试（含 SPSC 队列多线程并发测试）
- CMake 构建系统，FetchContent 自动管理第三方依赖
- 头文件仅含声明，模板类通过显式实例化将实现放在 .cpp

## 快速开始

### 环境要求

- CMake 3.20+
- 支持 C++20 的编译器（GCC 11+ / Clang 14+ / MSVC 2022）
- Git（用于 FetchContent 下载依赖）

### 构建

```bash
mkdir build
cd build
cmake .. -G Ninja
cmake --build .
```

### 运行测试

```bash
ctest --output-on-failure
```

### 运行示例

```bash
./bin/hello_neoflux
```

## 最小示例

```cpp
#include <neoflux/neoflux.h>

using namespace neoflux;

std::shared_ptr<Widget> BuildHome(BuildContext& ctx) {
  auto root = std::make_shared<Container>();
  root->SetBackgroundColor({255, 255, 255, 255});

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

## 项目结构

```
neoflux/
├── CMakeLists.txt          # 根构建配置
├── LICENSE                 # AGPL-3.0 开源协议
├── README.md               # 英文文档
├── README-zh.md            # 中文文档（本文件）
├── .clang-tidy             # clang-tidy 规则
├── .clang-format           # 代码风格
├── .gitignore              # Git 忽略规则
├── cmake/                  # CMake 模块
├── thirdparty/             # 第三方依赖（FetchContent 自动下载到 _deps/）
├── include/neoflux/        # 公共头文件（仅声明）
│   ├── core/               # 环形队列、类型定义、工具
│   ├── widget/             # Widget 系统
│   ├── app/                # Application、EventLoop
│   └── render/             # 渲染层、命令、tgfx、GLFW
├── src/                    # 实现（.cpp）
├── tests/                  # GTest 单元测试
├── examples/               # 示例应用
└── docs/                   # 架构文档
```

## 移动端渲染

移动端不使用 GLFW，tgfx 直接渲染到平台提供的 Surface：

- **Android**：传入 `ANativeWindow*` 作为 `platform_surface`
- **iOS**：传入 `CAMetalLayer*` 或 `CAEAGLLayer*` 作为 `platform_surface`

```cpp
// 移动端初始化示例
app.Init(argc, argv, width, height, "NeoFlux", platform_surface);
```

桌面端 `platform_surface` 传 `nullptr`，框架自动创建 GLFW 窗口。