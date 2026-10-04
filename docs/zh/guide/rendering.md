# 渲染管线

## 两层架构

- **Application 层**：构建 Widget 树，Taitank 计算布局，录制 RenderCommand 到 RenderContext
- **Render 层**：从 SPSC 环形队列消费命令，tgfx 执行绘制

## 渲染命令

RenderCommand 支持以下类型：
- `kBeginFrame` / `kEndFrame`：帧边界
- `kDrawRect`：绘制矩形
- `kDrawRoundedRect`：绘制圆角矩形
- `kDrawText`：绘制文本（UTF-8）
- `kClipRect`：设置裁剪区域
- `kTranslate`：平移变换
- `kSave` / `kRestore`：保存/恢复渲染状态

## 桌面端

桌面与移动端是同一个形态：tgfx `Window` 持有 GPU 上下文与交换链，
`Surface::MakeFrom(context, window)` 逐帧获取表面，`context->submit()` 提交并呈现。
只有窗口**创建**是平台相关的：

| 后端 | tgfx Window | 原生句柄来源 |
|------|-------------|--------------|
| `TGFX_USE_OPENGL`（Linux） | `tgfx::EGLWindow::MakeFrom(XID)` | `glfwGetX11Window()` |
| `TGFX_USE_OPENGL`（Windows） | `tgfx::WGLWindow::MakeFrom(HWND)` | `glfwGetWin32Window()` |
| `TGFX_USE_METAL`（Apple） | `tgfx::MetalWindow::MakeFrom(CAMetalLayer*)` | GLFW NSWindow 的 content view |
| `TGFX_USE_VULKAN`（Windows） | `tgfx::VulkanWindow::MakeFrom(HWND)` | `glfwGetWin32Window()` |
| `TGFX_USE_D3D12`（Windows） | `tgfx::D3D12Window::MakeForHwnd(HWND)` | `glfwGetWin32Window()` |

GLFW 窗口以 `GLFW_NO_API` 创建；任何平台 NeoFlux 都不自管 GL/EGL/WGL 上下文。
tgfx 背后没有内置光栅器——tgfx 是唯一的渲染实现。

## 移动端

tgfx 直接渲染到平台提供的 Surface：
- Android：`tgfx::EGLWindow::MakeFrom(ANativeWindow*)`
- iOS：`tgfx::EAGLWindow::MakeFrom(CAEAGLLayer*)`（需要 ObjC++ 应用壳，尚未接线）

## 下一步

- [架构](./architecture)
- [跨平台](./cross-platform)
