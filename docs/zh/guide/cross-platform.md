# 跨平台

## 支持平台

- **桌面端**：Windows / Linux / macOS
- **移动端**：Android / iOS

## 平台宏

| 宏 | 说明 |
|----|------|
| `NEOFLUX_PLATFORM_DESKTOP` | 桌面端平台 |
| `NEOFLUX_PLATFORM_MOBILE` | 移动端平台 |
| `NEOFLUX_PLATFORM_WINDOWS` | Windows |
| `NEOFLUX_PLATFORM_LINUX` | Linux |
| `NEOFLUX_PLATFORM_MACOS` | macOS |

## 桌面端

使用 GLFW 创建窗口与 OpenGL 上下文。`CMAKE_WIN32_EXECUTABLE` 确保 Windows 下不显示控制台窗口。

## 移动端

不使用 GLFW，tgfx 直接渲染到平台提供的 Surface：

```cpp
// 移动端初始化
app.Init(argc, argv, width, height, "NeoFlux", platform_surface);
```

:::tip
移动端将平台 Surface（Android 传 `ANativeWindow*`，iOS 传 `CAMetalLayer*`/`CAEAGLLayer*`）作为 `platform_surface` 参数传给 `Application::Init`。桌面端传 `nullptr`，GLFW 自动创建窗口。
:::

桌面端 `platform_surface` 传 `nullptr`，框架自动创建 GLFW 窗口。

## 构建系统

CMake 构建，FetchContent 管理依赖，所有第三方库放在 `thirdparty/` 下。

## 下一步

- [渲染管线](./rendering)
- [配置](./configuration)
