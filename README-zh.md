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
- **Render 层**：从 SPSC 环形队列消费命令，重放为 tgfx 绘制调用。所有平台统一由 tgfx `Window` 持有 GPU 上下文与交换链（按构建为 EGL/WGL/Metal/Vulkan/D3D12）；GLFW / 移动端 bridge 只负责承载原生窗口与输入。
- **SPSC 环形队列**：无锁单生产者单消费者有界环形缓冲区，位运算回绕，无需互斥锁。

## 特性

- 类 Flutter Widget 系统（StatelessWidget / StatefulWidget / State）
- 基于路由的 Widget 注册与导航
- Taitank flexbox 布局引擎（Container 支持 flex direction、padding、margin、justify/align）
- 完整指针事件管道：OnPointerDown/Up/Move/Enter/Exit，命中测试缓存
- 可拖拽 Widget（Draggable）与可滚动视图（ScrollView，支持滚轮与拖拽滚动）
- C++20 协程：Task\<void\>、Yield()、Sleep()，事件循环驱动
- 轻量状态机 + 协程"条件锁"模式
- 跨平台：详见下方[平台支持](#平台支持)（Linux / Windows 已通过 CI 验证）
- C++20 标准，使用 `std::string_view`、designated initializers 等现代特性
- 遵循 Google C++ 编码规范，clang-tidy 静态分析，-Werror 零警告
- GLog 日志 + GFlags 命令行参数解析
- GTest 单元测试
- CMake 构建系统，第三方依赖以 git submodule 形式内置（离线构建、版本锁定）
- 头文件仅含声明，模板类通过 `.inc` + 显式实例化将实现放在 `.cpp`

## 平台支持

| 平台 | 状态 | 说明 |
|------|------|------|
| Linux (x86-64) | ✅ 已验证 | CI 构建 + Xvfb 无头测试 |
| Windows (MSVC x64) | ✅ 已验证 | CI 构建 + 测试 |
| macOS | 🚧 使用 tgfx Metal，仍有 Apple 后端限制 | macOS 已纳入 CI 构建与测试矩阵。Apple 构建使用 tgfx `MetalWindow`，文字/矢量渲染走 CoreGraphics（`TGFX_USE_FREETYPE=OFF`）；剩余兼容性限制来自 tgfx 的 Apple 后端，而非缺少 CI 覆盖。 |
| Android | 🚧 渲染与输入已接线，缺应用壳 | tgfx `EGLWindow` 持有 EGL 上下文并渲染到 `ANativeWindow`；`MobileBridge` 把触摸输入分发进控件树。仍需一个创建 Surface 并转发触摸事件的 Android 应用壳（NativeActivity/JNI）。 |
| iOS | 🚧 需要应用壳 | `EAGLWindow::MakeFrom(CAEAGLLayer*)` 路径已定义，但本仓库尚无 ObjC++ 应用壳（视图层级）；`TgfxRenderer::Init` 会打日志并显式失败。 |

> 「已验证」指该平台的构建与测试在 CI 中通过。移动端渲染与触摸输入已通过
> tgfx 自身的平台抽象（EGL/EAGL Window）与 `PlatformBridge` 输入桥接入；
> 缺少的是平台应用壳（Android NativeActivity/JNI 工程、iOS ObjC++ 工程），
> 由它持有生命周期并交出原生 Surface——详见 [移动端渲染](#移动端渲染)。
> 移动端媒体播放 `CreateMediaPlayer()` 返回 `nullptr` 并带响亮告警。

## 快速开始

### 环境要求

- CMake 3.20+
- 支持 C++20 的编译器（GCC 11+ / Clang 14+ / MSVC 2022）
- Git（用于拉取 submodule 依赖，`git clone --recurse-submodules`）

### 构建

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

### 运行测试

测试默认禁用，需通过 CMake 选项启用：

```bash
cmake -S . -B build -G Ninja -DNEOFLUX_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

默认构建会生成快速开始应用 `build/bin/neoflux_app`。独立示例默认不构建，启用方式：

```bash
cmake -S . -B build -G Ninja -DNEOFLUX_BUILD_EXAMPLES=ON
cmake --build build
./build/bin/hello_neoflux
```

启用后还会构建 `counter`、`flex_demo`、`font_demo`、`scroll_demo`、
`loading_demo`、`drag_demo` 和 `media_player_demo`。

## 配置（gflags）

NeoFlux 使用 gflags 进行运行时配置，所有参数均为可选。

| 参数                          | 类型   | 默认值    | 说明                                                                 |
|-------------------------------|--------|-----------|----------------------------------------------------------------------|
| `--target_fps`                | int    | `60`      | 应用事件循环与渲染的目标帧率。                                       |
| `--idle_fps`                  | int    | `15`      | 空闲心跳帧率。当连续数帧无渲染请求且无协程/定时器任务时，事件循环降至此帧率以省电（输入事件立即唤醒）。`0` 表示禁用空闲降频。 |
| `--render_queue_capacity`     | int    | `2048`    | Application 层与 Render 层之间 SPSC 无锁环形队列容量，自动向上取整为 2 的幂。 |
| `--render_queue_drop_log_max` | int    | `10`      | 命令队列满导致丢弃时的告警最大打印次数，超出后静默计数。             |
| `--native_tuning`             | bool   | `true`    | 平台原生调优层总开关（线程调度、MMCSS、定时器精度、大核绑定）。`false` 时所有 native 入口均为 no-op。 |
| `--native_render_rt_priority` | int    | `1`       | Linux/Android：渲染线程尝试的 SCHED_FIFO 实时优先级（1 为最低，范围 1..99）。 |
| `--native_thread_nice`        | int    | `-5`      | Linux/Android：渲染线程（降级路径）与 UI 线程尝试的 nice 值（范围 -20..19）。 |
| `--native_bigcore_threshold_permille` | int | `950`  | 大核判定阈值（相对最快核最大频率的千分比，500..1000）。             |
| `--native_mmcss_profile`      | string | `Games`   | Windows：渲染线程注册 MMCSS 时使用的 profile 名称。                  |
| `--native_timer_period_ms`    | int    | `1`       | Windows：通过 timeBeginPeriod 请求的定时器精度（毫秒，`0` 表示不请求）。修复 CV 等待 ~15.6ms 粒度导致的帧率抖动。 |
| `--verbose_logging`           | bool   | `false`   | 启用详细 VLOG(1) 输出并将日志镜像到 stderr，用于调试。               |
| `--logtostderr`               | bool   | `false`   | 将日志输出到 stderr 而非日志文件。                                   |
| `--log_dir`                   | string | `./logs`  | 日志文件存放目录，不存在时自动创建。                                 |

GPU 后端**不是**运行时参数，也不是 NeoFlux 的概念：渲染层完全构建在 tgfx 之上，后端由 tgfx 自己的 `TGFX_USE_*` CMake 开关选定（每次构建恰好一个；Apple 之外默认 `TGFX_USE_OPENGL=ON`，Apple 默认 Metal），详见 [GPU 后端](#gpu-后端编译期选择)。

默认日志输出到 `./logs/` 文件，Windows 下不显示控制台窗口（`CMAKE_WIN32_EXECUTABLE`）。调试时使用 `--logtostderr --verbose_logging`。

### GPU 后端（编译期选择）

GPU 后端由 tgfx 负责。configure 阶段使用 tgfx 的原生开关；`thirdparty/CMakeLists.txt` 复刻 tgfx 的消解规则（固定优先级：VULKAN > D3D12 > METAL > OPENGL）并导出唯一的 `TGFX_USE_*=1` 宏，保证 NeoFlux 源码与 tgfx 实际编译的后端一致：

```bash
cmake -S . -B build -G Ninja                                # 默认：OpenGL（Linux/Windows/Android）
cmake -S . -B build -G Ninja -DTGFX_USE_METAL=ON -DTGFX_USE_OPENGL=OFF   # Apple
cmake -S . -B build -G Ninja -DTGFX_USE_D3D12=ON -DTGFX_USE_OPENGL=OFF   # Windows
```

| tgfx 后端 | 表面获取 | 状态 |
|------|------|------|
| `TGFX_USE_OPENGL` | Linux `tgfx::EGLWindow`（X11）/ Windows `tgfx::WGLWindow` / Android `tgfx::EGLWindow` | ✅ 已实现（Apple 之外的默认），桌面已在 Linux CI 验证 |
| `TGFX_USE_METAL` | `tgfx::MetalWindow`，呈现到 GLFW NSWindow 的 `CAMetalLayer` | 🚧 Apple 默认；macOS CI 会构建并测试此配置。Apple 文字/矢量渲染使用 CoreGraphics，而非 FreeType。 |
| `TGFX_USE_VULKAN` | `tgfx::VulkanWindow`（Win32 HWND） | 🚧 代码路径已存在，尚未在 CI 验证 |
| `TGFX_USE_D3D12` | `tgfx::D3D12Window::MakeForHwnd` | 🚧 代码路径已存在，尚未在 CI 验证 |

> 任何情况下 NeoFlux 都不自管 GL/EGL/WGL 上下文：GLFW 窗口以 `GLFW_NO_API` 创建，上下文与交换链由 tgfx `Window` 持有，`context->submit()` 完成呈现。tgfx 是**必备**依赖（不存在没有 tgfx 的构建）：请先初始化 submodule（`git submodule update --init --recursive`），`tgfx` 目标缺失时 configure 会直接报错。tgfx 的 CMake 会校验平台支持（如 D3D12 仅限 Windows、Metal 仅限 Apple），Apple + OpenGL 组合会被直接拒绝并给出原因。

## 字体系统

NeoFlux 在启动时扫描 `assets/fonts/`（以及相邻的构建目录相对路径）中的 TrueType（`.ttf`）、OpenType（`.otf`）和 TrueType Collection（`.ttc`）文件。Widget 通过文件名（不含扩展名）引用字体：

```cpp
auto text = std::make_shared<Text>("Hello World");
text->SetFont("NotoSansSC-Regular");  // 加载 assets/fonts/NotoSansSC-Regular.ttf
```

若 Widget 未指定字体，则使用第一个被发现的字体作为默认字体。将字体文件放入 `assets/fonts/` 目录即可通过名称引用。

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

1. `GlfwBridge` 接收 GLFW 鼠标事件（按钮、移动、滚轮）并通过回调转发。
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

### flex_demo

布局展示示例，演示 Taitank flex 布局：row/column 方向、居中对齐、flex grow、row reverse，使用彩色方块可视化布局效果。

### font_demo

字体系统演示：默认字体、显式 `SetFont()` 选择字体、多种字号/颜色、CJK 文本渲染。将字体放入 `assets/fonts/` 后按名称引用即可。

### scroll_demo

`ScrollView` 演示：标题栏 + 可滚动的彩色列表。支持滚轮滚动与拖拽滚动，内容自动裁剪到视口。

### loading_demo

演示 Widget 状态机与 C++20 协程的集成。"Start Loading" 按钮将 Widget 切换到加载状态；协程在约 2 秒内将进度条从 0% 动画到 100%，每帧 yield 一次。完成后 Widget 切换到成功状态。

### drag_demo

演示 `Draggable` Widget 与指针事件，以及"状态机作为条件锁"模式。彩色方块可拖拽；状态标签显示当前状态（Idle/Hovering/Dragging）和偏移量。指针按下时启动长按检测协程；如果 500ms 内释放，协程观察到状态变化后静默返回；如果按住超过 500ms，显示 "[Long Press!]" 指示器。

## 项目结构

```
NeoFlux/
├── CMakeLists.txt          # 构建框架与 neoflux_app
├── src/                    # 快速开始应用（路由与视图）
├── neoflux/                # 框架静态库
│   ├── include/neoflux/    # 公共头文件
│   ├── src/                # 框架实现
│   └── tests/              # 框架单元测试
├── examples/               # 可选示例应用
├── assets/fonts/           # 运行时字体
├── thirdparty/             # 第三方依赖
├── cmake/                  # CMake 模块
└── docs/                   # VitePress 文档
```

## 移动端渲染

移动端不使用 GLFW。tgfx 直接渲染到平台提供的 Surface，GPU 上下文完全由 tgfx 持有：

- **Android**：应用壳传入 `ANativeWindow*` 作为 `platform_surface`；
  `TgfxRenderer` 用 `tgfx::EGLWindow` 包装它——EGL display/context/surface
  的创建与呈现（`context->submit()` 触发 `eglSwapBuffers`）都在 tgfx 内部。
- **iOS**：等价路径 `tgfx::EAGLWindow::MakeFrom(CAEAGLLayer*)` 需要 ObjC++
  应用壳（本仓库暂未包含）。

触摸输入由 `MobileBridge`（`PlatformBridge` 的移动端实现）分发：应用壳在
JNI/UI 线程调用其 `DispatchTouchEvent()`，`Application::Init` 把回调接到
`DispatchPointerEvent()`——与桌面鼠标路径共用同一套命中测试管线，按钮类型
为 `MouseButton::kTouch`。

```cpp
// 移动端初始化示例（应用壳内）
app.Init(argc, argv, width, height, "NeoFlux", platform_surface);
```

桌面端 `platform_surface` 传 `nullptr`，框架自动创建 GLFW 窗口（`GLFW_NO_API`），随后与移动端完全一致地基于其原生句柄构建 tgfx `Window`（Linux/X11 用 `EGLWindow`、Windows 用 `WGLWindow`、Apple 用 `MetalWindow`）。任何平台都不存在 NeoFlux 自管的 GL 上下文：上下文、Surface、交换链与呈现全部归 tgfx。
