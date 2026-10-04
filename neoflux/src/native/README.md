# src/native/ — 平台原生调优层

**本目录是框架代码中唯一允许直接调用操作系统底层 API 的地方。**

## 放什么

| 内容 | 文件 |
|---|---|
| 接口声明 | `native_tuning.h`（`neoflux::native` 命名空间） |
| Windows 调优 | `windows/platform_win32.cpp`（线程优先级、`timeBeginPeriod(1)`、cpuid/xgetbv） |
| Linux/Android 调优 | `linux/platform_linux.cpp`（SCHED_FIFO→nice 降级、cpuid/getauxval） |
| macOS/iOS 调优 | `apple/platform_apple.cpp`（QoS class、sysctl） |
| 其他平台兜底 | `platform_common.cpp`（全部 no-op） |

当前能力：渲染线程/UI 线程调度整形、帧节拍定时器精度、CPU SIMD 特性检测（SSE4.2 / AVX2 / ASIMD / FP16，含 OS 支持校验）。

## 不放什么（历史教训）

- **任何渲染 / GPU API**——GPU 后端代码全部位于 tgfx 之后，由
  `NEOFLUX_BACKEND` 编译期宏分发（见 `thirdparty/CMakeLists.txt`）。
  曾经出现在这里的 `gl/gl_functions.{h,cpp}`（自管 GL 加载器）是
  架构错位的死代码，已在 `refactor_strip-gl-from-core` 中删除。
- **媒体解码 / 视频互操作**——属于 GL 后端模块（`src/media/`）。
- **窗口系统**——属于 `src/renderers/glfw_bridge.cpp` 或 `mobile_bridge.cpp`。

## 约定

- 所有入口 **best-effort**：权限不足/平台不支持时静默降级并记一条日志，
  绝不抛异常、绝不使调用方失败。
- 新增调优能力时：先在 `native_tuning.h` 定义跨平台语义，
  再在四个实现里补齐，兜底实现必须同步更新。
- CMake 侧按平台自动选择实现（`neoflux/CMakeLists.txt` 中
  "Platform-native tuning layer" 块），无需手动指定。
