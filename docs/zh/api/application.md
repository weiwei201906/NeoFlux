# Application

框架入口类，管理窗口、事件循环、路由栈和渲染层。

## 概述

`Application` 是 NeoFlux 的核心类，负责初始化平台窗口、启动事件循环、管理路由导航栈，并协调 Application 层与 Render 层的通信。

## 基本用法

```cpp
Application app;
app.Init(argc, argv, 800, 600, "My App");
app.PushRoute("/");
app.Run();
```

## 方法

| 方法 | 说明 |
|------|------|
| `Init(argc, argv, width, height, title, platform_surface)` | 初始化应用，创建窗口和渲染层 |
| `Run()` | 启动事件循环，阻塞直到窗口关闭 |
| `Stop()` | 停止事件循环和渲染层 |
| `PushRoute(route_name)` | 压入路由到导航栈 |
| `PopRoute()` | 弹出当前路由 |
| `GetRootWidget() -> Widget*` | 获取当前路由的根 Widget |
| `GetEventLoop() -> EventLoop&` | 获取事件循环引用 |

## 事件分发

Application 负责将平台事件分发给 Widget 树：
- `DispatchPointerEvent(pos, is_down)`：指针按下/释放
- `DispatchPointerMove(pos)`：指针移动
- `DispatchScrollEvent(xoffset, yoffset)`：滚轮滚动

## 另见

- [EventLoop](./event-loop)
- [Widget](./widget)
