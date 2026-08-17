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

通过 GLFW 创建窗口与 OpenGL 上下文，tgfx 渲染到 GL framebuffer，最后交换缓冲区。

## 移动端

tgfx 直接渲染到平台提供的 Surface：
- Android：`ANativeWindow*`
- iOS：`CAMetalLayer*` / `CAEAGLLayer*`

## 下一步

- [架构](./architecture)
- [跨平台](./cross-platform)
