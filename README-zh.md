# NeoFlux
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/compiler_support)
[![License](https://img.shields.io/badge/license-GPLv3-green.svg)](LICENSE)

一个跨平台 C++20 UI 框架，采用类 Flutter 的 Widget 开发模型。

> 本项目采用 **GNU General Public License v3.0 (GPL-3.0)** 开源协议。详见 [LICENSE](LICENSE) 文件。
>
> English documentation: [README.md](README.md)

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

- **Application 层**：运行业务逻辑，构建 Widget 树，通过 Taitank flexbox 引擎计算布局，记录渲染命令。
- **Render 层**：从 SPSC 环形队列消费命令，使用 tgfx 执行绘制。移动端 tgfx 直接渲染到平台 Surface，桌面端通过 GLFW 创建窗口 + OpenGL 上下文。
- **SPSC 环形队列**：无锁单生产者单消费者有界环形缓冲区，位运算回绕，无需互斥锁。

## 特性

- 类 Flutter Widget 系统（StatelessWidget / StatefulWidget / State）
- 基于路由的 Widget 注册与导航
- Taitank flexbox 布局引擎（Container 支持 flex direction、padding、margin、justify/align）
- 鼠标/触摸输入事件管道（命中测试 + 指针事件分发）
- 跨平台：Windows / Linux / macOS（桌面），Android / iOS（移动）
- C++20 标准，使用 `std::string_view`、designated initializers 等现代特性
- 遵循 Google C++ 编码规范，clang-tidy 静态分析
- GLog 日志 + GFlags 命令行参数解析
- GTest 单元测试
- CMake 构建系统，FetchContent 自动管理第三方依赖
- 头文件仅含声明，模板类通过 `.inc` + 显式实例化将实现放在 `.cpp`

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
# 完整演示（路由导航、状态管理、按钮回调）
./bin/hello_neoflux

# 极简计数器
./bin/counter
```

## 配置（gflags）

NeoFlux 使用 gflags 进行运行时配置，所有参数均为可选。

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `--target_fps` | int | `60` | 应用事件循环与渲染的目标帧率。 |
| `--render_queue_capacity` | int | `2048` | Application 层与 Render 层之间 SPSC 无锁环形队列容量，自动向上取整为 2 的幂。 |
| `--font_path` | string | `""` | TrueType/OpenType 字体文件路径（`.ttf`/`.ttc`/`.otf`）。设置后优先加载，否则回退到平台默认字体。 |
| `--verbose_logging` | bool | `false` | 启用详细 VLOG(1) 输出并将日志镜像到 stderr，用于调试。 |
| `--logtostderr` | bool | `false` | 将日志输出到 stderr 而非日志文件。 |
| `--log_dir` | string | `./logs` | 日志文件存放目录，不存在时自动创建。 |

默认日志输出到 `./logs/` 文件，Windows 下不显示控制台窗口（MinGW `-mwindows`）。调试时使用 `--logtostderr --verbose_logging`。

## Widget 系统

### 核心 Widget

| Widget | 说明 |
|--------|------|
| `Widget` | 抽象基类，重写 `Build()`、`OnMeasure()`、`Paint()`。 |
| `Container` | Flexbox 容器，支持 padding、margin、背景色、flex direction。 |
| `Text` | 单行文本，支持字体大小、颜色、对齐方式，UTF-8 编码。 |
| `Button` | 可点击按钮，支持标签、按下回调、按下状态样式。 |
| `StatelessWidget` | 无状态 Widget 基类。 |
| `StatefulWidget` | 有状态 Widget 基类，配合 `State<W>` 使用。 |

### 布局（Taitank Flexbox）

`Container` 暴露 flexbox 属性，直接映射到 Taitank：

```cpp
auto col = std::make_shared<Container>();
col->SetFlexDirection(FlexDirection::kColumn)   // 子控件垂直排列
   .SetJustifyContent(HAlign::kCenter)          // 主轴居中
   .SetAlignItems(VAlign::kCenter)              // 交叉轴居中
   .SetPadding({.left = 16, .top = 16, .right = 16, .bottom = 16})
   .SetBackgroundColor({.r = 245, .g = 245, .b = 250, .a = 255});
```

叶子控件（`Text`、`Button`）通过 `OnMeasure()` 报告固有尺寸，Taitank 布局时调用。

### 输入处理

鼠标/触摸事件从平台桥接流经 Widget 树：

1. `GlfwBridge` 接收 GLFW 鼠标事件，通过 `InputEventCallback` 转发。
2. `Application` 执行递归 `HitTest()`，找到光标下最深层的 Widget。
3. 调用命中 Widget 的 `OnPointerDown()` / `OnPointerUp()`，传入局部坐标。
4. `Button` 重写这些方法跟踪按下状态并触发 `on_pressed` 回调。

### 路由导航

Widget 通过 `RouteRegistry` 注册，压入/弹出导航栈：

```cpp
RouteRegistry::Instance().RegisterRoute("/settings", BuildSettingsPage);
app.PushRoute("/settings");  // 构建并显示设置页面
app.PopRoute();              // 返回上一路由
```

## 字体

NeoFlux 支持 UTF-8 文本渲染，字体搜索优先级：

1. `--font_path` 命令行参数
2. `thirdparty/fonts/` 目录（打包字体，详见该目录下的 README）
3. 平台系统字体（Windows 优先 `simhei.ttf`，Linux 优先 Noto Sans CJK，macOS 优先 PingFang）

如需开箱即用的 CJK 支持，将开源字体（如 Noto Sans SC、思源黑体）放入 `thirdparty/fonts/` 目录即可。

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

## 示例

### hello_neoflux

完整演示，包含有状态 Widget、按钮回调、路由导航、flex 布局。

### counter

极简计数器，演示 `StatefulWidget`、`Button` 回调、flex row/column 布局。

## 项目结构

```
neoflux/
├── CMakeLists.txt          # 根构建配置
├── LICENSE                 # GPL-3.0 开源协议
├── README.md               # 英文文档
├── README-zh.md            # 中文文档（本文件）
├── .clang-tidy             # clang-tidy 规则
├── .clang-format           # 代码风格
├── cmake/                  # CMake 模块
├── thirdparty/             # 第三方依赖
│   ├── fonts/              # 打包字体目录
│   └── CMakeLists.txt      # FetchContent 配置
├── include/neoflux/        # 公共头文件（仅声明）
│   ├── core/               # 环形队列、类型定义、工具
│   ├── widget/             # Widget 系统
│   ├── app/                # Application、EventLoop
│   └── render/             # 渲染层、命令、tgfx、GLFW
├── src/                    # 实现（.cpp）
├── tests/                  # GTest 单元测试
├── examples/               # 示例应用（hello_neoflux、counter）
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
