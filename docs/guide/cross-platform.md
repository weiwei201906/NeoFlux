# Cross-Platform

NeoFlux is designed to run on both desktop and mobile platforms with a single
codebase.

## Platform Detection

CMake detects the target platform and sets preprocessor macros:

| Macro | Platform |
|-------|----------|
| `NEOFLUX_PLATFORM_DESKTOP` | Windows, Linux, macOS |
| `NEOFLUX_PLATFORM_MOBILE` | Android, iOS |
| `NEOFLUX_PLATFORM_WINDOWS` | Windows |
| `NEOFLUX_PLATFORM_LINUX` | Linux |
| `NEOFLUX_PLATFORM_MACOS` | macOS |

## Desktop (GLFW + OpenGL)

On desktop platforms, NeoFlux uses:

- **GLFW** for window creation, input handling, and OpenGL context management
- **OpenGL 3.3** for rendering

The `GlfwBridge` class encapsulates all platform-specific window operations:

```cpp
#ifdef NEOFLUX_PLATFORM_DESKTOP
  // GLFW implementation
#else
  // Mobile stub
#endif
```

### Window Resizing

Windows are resizable by default. When the window is resized:

1. GLFW fires a framebuffer size callback.
2. The application updates `window_width_` / `window_height_`.
3. `MarkFrameDirty()` triggers a relayout.
4. The render layer queries the new size each frame in `BeginFrame`.

### DPI Handling

On Windows with DPI virtualization, the logical window size may differ from the
physical framebuffer size. NeoFlux handles this by:

- Using `glfwGetFramebufferSize` for `glViewport` (physical pixels)
- Using `glfwGetWindowSize` for `u_resolution` (logical/layout pixels)
- Scaling input coordinates from logical to layout space

## Mobile (tgfx)

On mobile platforms, NeoFlux uses:

- **tgfx** for 2D rendering (backs onto Vulkan/Metal/GLES)
- Platform-specific window management (provided by the host app)

The `MobileBridge` class provides a stub interface that the mobile host
application implements.

### Android

- tgfx renders to an `ANativeWindow`
- Input events come from the Android touch system
- Fonts are bundled in the APK assets

### iOS

- tgfx renders to a `CAMetalLayer` or `CAEAGLLayer`
- Input events come from UIKit gesture recognizers
- Fonts are bundled in the app bundle

## Writing Cross-Platform Code

Use platform macros for platform-specific code:

```cpp
void PlatformInit() {
#ifdef NEOFLUX_PLATFORM_WINDOWS
  // Windows-specific initialization
#elif defined(NEOFLUX_PLATFORM_LINUX)
  // Linux-specific initialization
#elif defined(NEOFLUX_PLATFORM_MACOS)
  // macOS-specific initialization
#endif
}
```

For rendering, use the `RenderContext` abstraction, which hides the underlying
graphics API:

```cpp
void MyWidget::Paint(RenderContext& context) {
  context.DrawRect(bounds_, color);  // works on all platforms
}
```

## CMake Configuration

The root `CMakeLists.txt` detects the platform and configures accordingly:

```cmake
if(WIN32)
  target_compile_definitions(neoflux PRIVATE NEOFLUX_PLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
  target_compile_definitions(neoflux PRIVATE NEOFLUX_PLATFORM_LINUX)
elseif(APPLE)
  target_compile_definitions(neoflux PRIVATE NEOFLUX_PLATFORM_MACOS)
endif()
```

Mobile builds require additional toolchain configuration and are typically
integrated into the platform's native build system (Gradle for Android, Xcode
for iOS).
