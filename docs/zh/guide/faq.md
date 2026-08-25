# 常见问题

## 基础

### NeoFlux 是什么？

NeoFlux 是一个轻量级、跨平台的 C++20 UI 框架，采用两层架构：Application 层运行业务逻辑和 Taitank flex 布局引擎，Render 层通过 tgfx（移动端）或 GLFW（桌面端）消费渲染命令。

### 为什么用 C++20？

C++20 提供了协程（`co_await`）、concepts、`std::span`、指定初始化器和 `std::bit_ceil`。框架全程使用这些特性实现简洁高效的代码。

### NeoFlux 可以用于生产环境吗？

NeoFlux 正在积极开发中。核心架构（Widget 树、flex 布局、渲染管线、输入分发）已可用。当前限制请查看 GitHub issues。

## 构建

### 如何构建 NeoFlux？

```bash
cmake -S . -B build
cmake --build build -j
```

示例和测试默认关闭。启用它们：

```bash
cmake -S . -B build -DNEOFLUX_BUILD_EXAMPLES=ON -DNEOFLUX_BUILD_TESTS=ON
```

### 为什么第一次构建很慢？

NeoFlux 使用 FetchContent 下载并构建第三方依赖（glog、gflags、glfw、taitank、freetype、gtest）。后续构建很快，因为依赖缓存在 `thirdparty/_deps/` 中。

### 如何交叉编译？

为目标平台设置 CMake 工具链文件。NeoFlux 使用平台宏（`NEOFLUX_PLATFORM_DESKTOP`、`NEOFLUX_PLATFORM_MOBILE`、`NEOFLUX_PLATFORM_WINDOWS` 等）选择合适的后端。

## 渲染

### 支持哪些渲染后端？

- **Vulkan**（默认）— 通过 tgfx
- **OpenGL** — 通过 tgfx/GLFW
- **CPU** — 软件渲染（回退）

通过 `--render_backend=vulkan|gl|cpu` 选择。

### 为什么窗口是黑的？

常见原因：
1. **未加载字体** — 文本 Widget 需要字体目录中有 `.ttf`/`.otf` 文件。在 `Init()` 前调用 `app.SetFontDir("./fonts/")`。
2. **未推送路由** — 所有示例都需要 `RouteRegistry::RegisterRoute()` 然后 `app.PushRoute("/")`。
3. **窗口未暴露** — 某些平台上第一帧可能需要 resize 或 focus 事件。

### 为什么文字是乱码？

字体文件不包含你尝试渲染的字形。使用支持你字符集的字体（如中文用 NotoSansSC）。

## Widget

### 如何创建自定义 Widget？

继承 `Widget`（或 `StatelessWidget`/`StatefulWidget`）并重写相关虚函数：`Build()`、`OnMeasure()`、`Paint()` 和事件处理函数。参见 [Widget 系统](./widgets) 指南。

### `MarkNeedsBuild()` 和 `MarkFrameDirty()` 有什么区别？

- `MarkNeedsBuild()` — 标记 Widget 在下一帧重建（子节点可能变化）。
- `MarkFrameDirty()` — 触发布局/绘制周期但不重建 Widget。用于绘制时变化（如拖拽偏移、滚动位置）。

### 为什么我的 Draggable 不跟随光标？

确保不要在 `OnPointerMove()` 中调用 `MarkNeedsBuild()` — 拖拽偏移仅在绘制时使用。框架在指针事件时自动调用 `MarkFrameDirty()`。

## 布局

### 为什么我的 Widget 不可见？

- Widget 尺寸为零（检查 `OnMeasure()` 返回值）。
- Widget 在视口外（检查 flex 约束）。
- Widget 被 `ScrollView` 或 `ClipRect` 裁剪。

### 如何让 Widget 填满可用空间？

使用 `Expanded`（设置 `flex_grow`），或在 flex 容器中设置 `SetWidth(0)` 配合 `flex_grow > 0`。

## 线程

### NeoFlux 是线程安全的吗？

Widget 树只在主线程访问。渲染层在独立线程运行，通过 SPSC 环形队列通信。不要从渲染线程操作 Widget。

### 协程如何工作？

NeoFlux 提供 `Task<T>` 协程类型，支持 `co_await Yield()`（下一帧）和 `co_await Sleep(duration)`。协程由 `EventLoop` 在主线程调度。使用 `std::weak_ptr` 防止 `co_await` 期间 Widget 被销毁。

## 故障排除

### 应用启动时崩溃

检查：
1. 字体目录存在且包含有效字体文件。
2. `Init()` 在 `Run()` 之前调用。
3. 路由已注册并推送。
4. 渲染后端在你的平台上受支持。

### clang-tidy 报告警告

运行 `clang-tidy -p build src/**/*.cpp` 检查。项目目标是零警告。完整检查清单参见 [贡献指南](./contributing)。

### 如何启用详细日志？

```bash
./my_app --verbose_logging=true
```

日志默认写入 `logs/` 目录。使用 `--logtostderr=true` 输出到 stderr。
