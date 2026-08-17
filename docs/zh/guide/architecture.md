# 架构

NeoFlux 采用两层架构，层间通过无锁环形队列通信。

## 架构图

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

## Application 层

运行业务逻辑，构建 Widget 树，通过 Taitank flexbox 引擎计算布局，记录渲染命令到 RenderContext。

核心组件：
- **Application**：框架入口，管理窗口、事件循环、路由栈
- **EventLoop**：CV 驱动的帧调度，协程调度器，定时器队列
- **RouteRegistry**：路由注册表，支持路由注册与导航栈
- **Widget Tree**：Widget 组成的 UI 树，Taitank 节点绑定

## Render 层

从 SPSC 环形队列消费命令，使用 tgfx 执行绘制。

- **桌面端**：通过 GLFW 创建窗口与 OpenGL 上下文，tgfx 渲染到 GL framebuffer
- **移动端**：tgfx 直接渲染到平台提供的 Surface（ANativeWindow / CAMetalLayer）

## SPSC 环形队列

无锁单生产者单消费者有界环形缓冲区：
- 容量运行时可配置（`--render_queue_capacity`），自动向上取整为 2 的幂
- 位运算回绕，缓存行对齐 head/tail，无需互斥锁
- Application 层生产 RenderCommand，Render 层消费执行

## 线程模型

- **UI 线程**：事件处理、布局计算、命令录制
- **渲染线程**：命令消费、tgfx 绘制、缓冲区交换
- 两层通过 SPSC 队列解耦，无锁竞争

## 下一步

- [Widget 系统](./widgets)
- [渲染管线](./rendering)
- [跨平台](./cross-platform)
