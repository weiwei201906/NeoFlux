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

### Android Surface 生命周期（EGL 上下文丢失恢复）

Android 在应用退到后台或屏幕关闭时可能销毁 `ANativeWindow`。应用回到前台时会提供新的 `ANativeWindow`。NeoFlux 优雅处理此过程：

```
应用退到后台              应用回到前台
     |                        |
     v                        v
OnSurfaceDestroyed()  OnSurfaceCreated(new_window)
     |                        |
     v                        v
eglDestroySurface()   eglCreateWindowSurface()
surface_valid_ = false surface_valid_ = true
渲染循环跳过 GL 调用    渲染线程被唤醒，
（仅排空队列）          恢复渲染
```

**核心设计：EGL 上下文被保留，仅重建 Surface。** 这意味着所有 GL 资源（纹理、着色器、程序、FBO）在后台/前台循环中全部存活。只有连接 GL 到显示器的窗口 Surface 被拆除并重建。

渲染循环每帧检查 `surface_valid_`。为 `false` 时，它排空命令队列但跳过所有 `renderer_->` 调用，防止 `EGL_BAD_SURFACE` 崩溃。

**宿主应用集成：**

```cpp
// 在 Android Activity 的 onPause() 中：
app->GetRenderLayer()->OnSurfaceDestroyed();

// 在 onSurfaceCreated() 中（新的 ANativeWindow*）：
app->GetRenderLayer()->OnSurfaceCreated(new_native_window);
```

:::warning
不要在后台时销毁 `Application` 或 `RenderLayer`。EGL 上下文和所有 GL 资源被有意保留。仅调用 `OnSurfaceDestroyed()` / `OnSurfaceCreated()`。
:::

## 构建系统

CMake 构建，FetchContent 管理依赖，所有第三方库放在 `thirdparty/` 下。

## 下一步

- [渲染管线](./rendering)
- [配置](./configuration)
