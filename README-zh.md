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
|  - Taitank Layout Engine  |         |  - PlatformBridge        |
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
- 完整指针事件管道：OnPointerDown/Up/Move/Enter/Exit，命中测试缓存
- 可拖拽 Widget（Draggable）与可滚动视图（ScrollView，支持滚轮与拖拽滚动）
- C++20 协程：`Task<void>`、`Yield()`、`Sleep()`，事件循环驱动，`shared_ptr` 生命周期管理
- 轻量状态机 + 协程"条件锁"模式
- 跨平台：Windows / Linux / macOS（桌面），Android / iOS（移动）
- 统一 PlatformBridge 接口：桌面端 GLFW，移动端 EGL/Metal
- C++20 标准，使用 `std::string_view`、designated initializers 等现代特性
- 遵循 Google C++ 编码规范，clang-tidy 静态分析，-Werror 零警告
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

测试和示例默认禁用，需通过 CMake 选项启用：

```bash
cmake -S . -B build -DNEOFLUX_BUILD_TESTS=ON 
cmake --build build
cd build && ctest --output-on-failure
```

### 运行示例

示例**默认关闭**，需在配置时启用：

```bash
cmake -S . -B build -DNEOFLUX_BUILD_EXAMPLES=ON
cmake --build build
./build/bin/hello_neoflux
```

其他示例：

```bash
./build/bin/counter         # 计数器
./build/bin/flex_demo       # flex 布局
./build/bin/font_demo       # 字体系统
./build/bin/scroll_demo     # 滚动视图
./build/bin/loading_demo    # 状态机 + 协程动画
./build/bin/drag_demo       # 可拖拽 Widget
```

## 配置（gflags）

NeoFlux 使用 gflags 进行运行时配置，所有参数均为可选。

| 参数                      | 类型   | 默认值    | 说明                                                                 |
|---------------------------|--------|-----------|----------------------------------------------------------------------|
| `--target_fps`            | int    | `60`      | 应用事件循环与渲染的目标帧率。                                       |
| `--render_queue_capacity` | int    | `2048`    | Application 层与 Render 层之间 SPSC 无锁环形队列容量，自动向上取整为 2 的幂。 |
| `--verbose_logging`       | bool   | `false`   | 启用详细 VLOG(1) 输出并将日志镜像到 stderr，用于调试。               |
| `--logtostderr`           | bool   | `false`   | 将日志输出到 stderr 而非日志文件。                                   |
| `--log_dir`               | string | `./logs`  | 日志文件存放目录，不存在时自动创建。                                 |
| `--render_backend`        | string | `vulkan`  | 渲染后端选择：`vulkan`、`gl`、`cpu`。Vulkan/CPU 尚未实现时回退到 OpenGL 并输出警告。 |

默认日志输出到 `./logs/` 文件，Windows 下不显示控制台窗口（`CMAKE_WIN32_EXECUTABLE`）。调试时使用 `--logtostderr --verbose_logging`。

## 字体系统

NeoFlux 使用字体管理器，在启动时扫描可配置的字体目录，支持 TrueType（`.ttf`）、OpenType（`.otf`）和 TrueType Collection（`.ttc`）文件。默认目录为 `fonts/`。

**在 `Init()` 之前配置字体目录：**

```cpp
Application app;
app.SetFontDir("./fonts/");  // 可选：覆盖默认的 "fonts"
app.Init(argc, argv, 800, 600, "NeoFlux");
```

Widget 通过文件名（不含扩展名）引用字体：

```cpp
auto text = std::make_shared<Text>("Hello World");
text->SetFont("NotoSansSC-Regular");  // 加载 <font_dir>/NotoSansSC-Regular.ttf
```

若 Widget 未指定字体，则使用第一个被发现的字体作为默认字体。相对路径从工作目录解析，并自动向上回退（`../`、`../../`）以适配构建子目录。

> **警告：** 如果配置的字体目录为空，文本渲染会失败或显示乱码。发布前务必至少放入一个字体文件（如渲染中文需 CJK 字体）。在 `Init()` 之前调用 `SetFontDir()` 指定自定义目录。
>
> **注意：** 运行示例（example）前，确保 `thirdparty/fonts/` 目录下有字体文件，否则文本会显示为乱码或空白。

### CMake 自动拷贝字体

在你自己的工程中，将字体放在 `fonts/` 目录下，通过 CMake 在构建时自动拷贝到输出目录：

```cmake
# CMakeLists.txt
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE neoflux)

# 构建时将 fonts/ 目录拷贝到可执行文件同级目录
add_custom_command(TARGET my_app POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/fonts $<TARGET_FILE_DIR:my_app>/fonts
)
```

然后在代码中配置：

```cpp
Application app;
app.SetFontDir("./fonts/");  // 对应拷贝到输出目录的 fonts/ 文件夹
app.Init(argc, argv, 800, 600, "My App");
```

## CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `NEOFLUX_BUILD_TESTS` | `OFF` | 构建单元测试（gtest）。 |
| `NEOFLUX_BUILD_EXAMPLES` | `OFF` | 构建示例应用。 |
| `NEOFLUX_ENABLE_CLANG_TIDY` | `OFF` | 将 clang-tidy 作为构建步骤运行。 |
| `NEOFLUX_USE_TGFX` | `OFF` | 使用 tgfx 渲染后端（Windows 上需要 MSVC）。 |
| `NEOFLUX_FFPLAY_PATH` | `""` | MediaWidget 使用的 ffplay 可执行文件路径。为空则运行时从 `PATH` 解析。 |

## 构建测试

测试和示例默认禁用，通过 `NEOFLUX_BUILD_TESTS` 和 `NEOFLUX_BUILD_EXAMPLES` CMake 选项启用：

```bash
cmake -S . -B build -DNEOFLUX_BUILD_TESTS=ON -DNEOFLUX_BUILD_EXAMPLES=ON
cmake --build build -j 16
cd build && ctest --output-on-failure
```

## 构建产物结构

```
build/bin/
├── *.exe              (9 个示例，每个约 1.8MB)
├── libglog.dll        (自动拷贝，与 exe 同目录)
├── libmpv-2.dll       (自动拷贝，与 exe 同目录)
└── fonts/
    └── NotoSansSC-Regular.ttf  (从 thirdparty/fonts/ 自动拷贝)
```

- **Windows**: DLL 放在可执行文件同目录（标准 Windows 部署方式）。无需启动脚本或 PATH 设置，双击 exe 即可运行。
- **Linux/macOS**: 示例设置 `RPATH=$ORIGIN/lib`，共享库相对于可执行文件查找。
- **字体**: 构建前运行 `examples/download_fonts.ps1`（Windows）或 `examples/download_fonts.sh`（Linux/macOS）下载 Noto Sans SC。字体文件不提交到 git。

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

## Widget 系统

### 核心 Widget

| Widget            | 说明                                                         |
|-------------------|--------------------------------------------------------------|
| `Widget`          | 抽象基类，重写 `Build()`、`OnMeasure()`、`Paint()` 及事件回调。 |
| `Container`       | Flexbox 容器，支持 padding、margin、背景色、圆角、flex direction。 |
| `Text`            | 单行文本，支持字体大小、颜色、对齐方式，UTF-8 编码。          |
| `Button`          | 可点击按钮，支持标签、按下回调、按下状态样式。                |
| `ScrollView`      | 可滚动视口，支持滚轮与拖拽滚动，内容裁剪。                    |
| `Draggable`       | 可拖拽容器，绘制时平移不影响 Taitank 布局。                   |
| `TextField`       | 单行可编辑文本输入，支持光标导航、占位符、UTF-8 和焦点管理。   |
| `MediaWidget`     | 基于 ffplay 子进程的媒体播放，跨平台，无需链接 FFmpeg。        |
| `Expanded`        | 设置了 flex_grow 的容器，填充父容器剩余空间。                 |
| `SizedBox`        | 固定宽高的容器，用于固定间距。                                |
| `StatelessWidget` | 无状态 Widget 基类。                                          |
| `StatefulWidget`  | 有状态 Widget 基类，配合 `State<W>` 使用。                    |

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

1. `PlatformBridge` 接收平台输入事件（GLFW 鼠标 / 移动端触摸）并通过回调转发。
2. `Application` 执行递归 `HitTest()` 找到光标下最深层的 Widget。命中测试缓存避免每次指针移动都遍历整棵树；布局变化时缓存自动失效。
3. 调用命中 Widget 的事件处理函数，传入局部坐标：
   - `OnPointerDown()` / `OnPointerUp()` — 按下与释放
   - `OnPointerMove()` — 悬停或拖拽时的光标移动
   - `OnPointerEnter()` / `OnPointerExit()` — 悬停进入/离开
4. `Button` 重写按下/释放跟踪状态并触发回调；`Draggable` 重写移动更新拖拽偏移；`ScrollView` 重写移动支持拖拽滚动。

### 路由导航

Widget 通过 `RouteRegistry` 注册，压入/弹出导航栈：

```cpp
RouteRegistry::Instance().RegisterRoute("/settings", BuildSettingsPage);
app.PushRoute("/settings");  // 构建并显示设置页面
app.PopRoute();              // 返回上一路由
```

> **提示：** 哪怕只有一个路由，也必须先注册再调用 `PushRoute`——`Init` 不会自动显示任何内容。

## 示例

### hello_neoflux

完整演示，包含有状态 Widget、按钮回调、路由导航、flex 布局。

### counter

极简计数器，演示 `StatefulWidget`、`Button` 回调、flex row/column 布局。

### flex_demo

布局展示示例，演示 Taitank flex 布局：row/column 方向、居中对齐、flex grow、row reverse，使用彩色方块可视化布局效果。

### font_demo

字体系统演示：默认字体、显式 `SetFont()` 选择字体、多种字号/颜色、CJK 文本渲染。将字体放入 `thirdparty/fonts/` 后按名称引用即可。

### scroll_demo

`ScrollView` 演示：标题栏 + 可滚动的彩色列表。支持滚轮滚动与拖拽滚动，内容自动裁剪到视口。

### loading_demo

演示 Widget 状态机与 C++20 协程的集成。"Start Loading" 按钮将 Widget 切换到加载状态；协程在约 2 秒内将进度条从 0% 动画到 100%，每帧 yield 一次。完成后 Widget 切换到成功状态。

### drag_demo

演示 `Draggable` Widget 与指针事件，以及"状态机作为条件锁"模式。彩色方块可拖拽；状态标签显示当前状态（Idle/Hovering/Dragging）和偏移量。指针按下时启动长按检测协程；如果 500ms 内释放，协程观察到状态变化后静默返回；如果按住超过 500ms，显示 "[Long Press!]" 指示器。

## 协程

NeoFlux 支持 C++20 协程用于异步工作。在事件循环上调度一个 `Task<void>`，它在就绪时的下一帧恢复：

```cpp
#include <neoflux/core/task.h>

neoflux::Task<void> AnimateAsync() {
  for (int i = 0; i < 60; ++i) {
    co_await neoflux::Yield();  // 下一帧恢复
    widget->SetOpacity(i / 60.0F);
  }
}

event_loop.Schedule(AnimateAsync());
```

协程由 `shared_ptr` 管理生命周期，`active_tasks_` map 保存所有活跃协程，确保定时器或 yield 等待中的句柄不会引用已销毁的 frame。

### Sleep

使用 `co_await Sleep(duration)` 将协程挂起指定时长。事件循环维护一个定时器队列（`std::multimap`，时间点到协程句柄的映射），每帧检查并恢复到期的定时器：

```cpp
neoflux::Task<void> LongPressDetector(std::weak_ptr<Button> weak_btn) {
  co_await neoflux::Sleep(std::chrono::milliseconds(500));
  auto btn = weak_btn.lock();
  if (!btn) co_return;          // Widget 已销毁
  if (btn->IsPressed()) {       // 状态机作为条件锁
    btn->OnLongPress();
  }
}
```

### 状态机 + 协程模式

Widget 携带轻量 `WidgetState`（Idle、Hovering、Dragging 等）。状态迁移是协程的"条件锁"：指针按下时启动的协程在睡眠后检查 Widget 状态；如果状态已改变（如指针已释放），协程静默返回。无需显式取消机制——状态机本身就是执行的门控。

> **警告：** 捕获 Widget 指针的协程必须使用 `std::weak_ptr`，并在每次 `co_await` 后重新 lock。Widget 可能在协程挂起于 `Sleep` 或 `Yield` 时被销毁；恢复后访问裸指针会导致 use-after-free。

## 移动端渲染

移动端不使用 GLFW，tgfx 直接渲染到平台提供的 Surface：

- **Android**：传入 `ANativeWindow*` 作为 `platform_surface`
- **iOS**：传入 `CAMetalLayer*` 或 `CAEAGLLayer*` 作为 `platform_surface`

```cpp
// 移动端初始化示例
app.Init(argc, argv, width, height, "NeoFlux", platform_surface);
```

桌面端 `platform_surface` 传 `nullptr`，框架自动创建 GLFW 窗口。

## 项目结构

```
neoflux/
├── CMakeLists.txt          # 根构建配置
├── LICENSE                 # GPL-3.0 开源协议
├── README.md               # 英文文档
├── README-zh.md            # 中文文档（本文件）
├── .clang-tidy             # clang-tidy 规则
├── .clang-format           # 代码风格
├── thirdparty/             # 第三方依赖（FetchContent）
│   ├── fonts/              # 字体目录（开发者自行放入）
│   └── CMakeLists.txt      # FetchContent 配置
├── include/neoflux/        # 公共头文件（仅声明）
│   ├── core/               # 环形队列、类型定义、协程、工具
│   ├── widget/             # Widget 系统
│   ├── app/                # Application、EventLoop
│   └── render/             # 渲染层、命令、tgfx、PlatformBridge
├── src/                    # 实现（.cpp）
├── tests/                  # GTest 单元测试
├── examples/               # 示例应用
└── docs/                   # VitePress 文档
```
