# Widget 系统

NeoFlux 采用类 Flutter 的 widget 模型：把 `shared_ptr<Widget>` 节点组合成树来构建 UI，
布局由 Taitank flexbox 引擎计算。本页介绍当前随框架发布的 widget 集合，以及 widget
状态机如何与 C++20 协程配合。

## Widget 集合

| Widget | 基类 | 作用 |
|--------|------|------|
| `Widget` | —— | 抽象基类。持有 Taitank 节点、bounds、子节点与生命周期状态。 |
| `Container` | `Widget` | Flexbox 盒子：flex 方向、justify/align、padding、margin、背景、圆角。 |
| `Text` | `Widget` | 叶子；用指定字体、字号、颜色、对齐渲染一段 UTF-8 字符串。 |
| `Button` | `Widget` | 可点击标签，带 `on_pressed` 回调与按下样式。 |
| `ScrollView` | `Widget` | 带裁剪的可滚动视口，滚轮或拖拽平移内容。 |
| `SizedBox` | `Container` | 强制固定宽/高（0/负数表示该轴自适应）。 |
| `Expanded` | `Container` | 在 flex 父容器中占满主轴剩余空间的子节点。 |
| `Draggable` | `Container` | 跟随指针的盒子；偏移在绘制时施加，布局不受影响。 |
| `RouteRegistry` | —— | 路由名 → widget 构建函数的单例表，驱动导航。 |

## Widget 生命周期

```
创建 -> Build -> 布局(Taitank) -> Paint(录制命令) -> 渲染线程绘制
            ^                                              |
            |____________ MarkNeedsBuild() / MarkFrameDirty() ____|
```

1. **Build** —— 脏 widget 从 `Build(BuildContext&)` 返回新的子树。
2. **布局** —— 根节点 `PerformLayout(width, height)` 跑 Taitank，把算好的矩形回写到每个 widget。
3. **Paint** —— 每个 widget 把绘制命令录入 `RenderContext`。
4. **提交** —— 命令经 SPSC 队列送到渲染线程。

`MarkNeedsBuild()` 标记一个子树待重建；`MarkFrameDirty()` 触发整帧的布局/重绘。

## Container

最主要的布局 widget，与一个 Taitank flex 节点一一对应：

```cpp
auto col = std::make_shared<Container>();
col->SetFlexDirection(FlexDirection::kColumn)
   .SetJustifyContent(HAlign::kCenter)
   .SetAlignItems(VAlign::kCenter)
   .SetPadding({.left = 16, .top = 16, .right = 16, .bottom = 16})
   .SetBackgroundColor({.r = 245, .g = 245, .b = 250, .a = 255})
   .SetBorderRadius(8.0F);
```

链式 setter 返回 `Container&`，可流式配置一个盒子。

## Text

通过 `OnMeasure()` 上报自身固有尺寸的叶子 widget：

```cpp
auto label = std::make_shared<Text>("Hello World");
label->SetFontSize(18.0F)
     .SetTextColor({.r = 0, .g = 0, .b = 0, .a = 255})
     .SetAlignment(HAlign::kCenter)
     .SetFont("NotoSansSC-Regular");  // 可选，按字体名
```

## Button

```cpp
auto btn = std::make_shared<Button>("Click Me");
btn->SetOnPressed([]() { LOG(INFO) << "pressed"; });
btn->SetFontSize(16.0F)
   .SetBackgroundColor({.r = 33, .g = 150, .b = 243, .a = 255})
   .SetTextColor({.r = 255, .g = 255, .b = 255, .a = 255});
```

`Button` 自己跟踪按下/抬起，仅在 widget 上真实按下-抬起时才触发 `on_pressed`。

## ScrollView

设置唯一内容子节点；超过视口的部分被裁剪，用滚轮或拖拽平移：

```cpp
auto scroll = std::make_shared<ScrollView>();

auto content = std::make_shared<Container>();
content->SetFlexDirection(FlexDirection::kColumn);
for (int i = 0; i < 20; ++i) {
  content->AddChild(std::make_shared<Text>("Item " + std::to_string(i)));
}

scroll->SetContent(content);
parent->AddChild(scroll);
```

## SizedBox

固定尺寸盒子（某轴传 0 表示该轴自适应）：

```cpp
parent->AddChild(std::make_shared<SizedBox>(0.0F, 16.0F));  // 16px 间距

auto card = std::make_shared<SizedBox>(200.0F, 120.0F);
card->SetBackgroundColor({.r = 240, .g = 240, .b = 240, .a = 255});
```

## Expanded

在 flex 父容器中占满主轴剩余空间（等价于 `Container` 的 `flex_grow = flex`）：

```cpp
auto row = std::make_shared<Container>();
row->SetFlexDirection(FlexDirection::kRow);

row->AddChild(std::make_shared<Expanded>(std::make_shared<Text>("Left")));
row->AddChild(std::make_shared<Expanded>(std::make_shared<Text>("Right"), /*flex=*/2));
// "Right" 占用的剩余空间是 "Left" 的两倍。
```

## Draggable

一种 `Container`，其内容跟随指针移动。拖拽偏移在绘制时施加（平移变换），
因此布局 bounds 保持不变 —— 盒子下方的 widget 不会跟着跳。观察
`OnStateChanged` 即可响应 `kIdle` / `kHovering` / `kDragging`。

## RouteRegistry 与导航

启动时为每个路由名注册一个构建函数，然后在导航栈上 push/pop：

```cpp
RouteRegistry::Instance().RegisterRoute("/home", BuildHomePage);
RouteRegistry::Instance().RegisterRoute("/settings", BuildSettingsPage);

app.PushRoute("/settings");  // 构建并显示设置页
app.PopRoute();              // 返回上一个路由（根路由拒绝弹出）
```

在 widget 内部，通过 `BuildContext`：

```cpp
Application* app = context.GetApplication();
nav_button->SetOnPressed([app] { app->PushRoute("/settings"); });
```

## 状态机 + 协程 `Sleep`

widget 通过 `SetState()` 携带一个轻量 `WidgetState`。内置状态：

| 状态 | 含义 |
|------|------|
| `kIdle` | 静止，无交互。 |
| `kHovering` | 指针悬停在 widget 上。 |
| `kDragging` | 指针捕获并移动中。 |
| `kLoading` | 忙碌 / 进行中。 |
| `kSuccess` | 操作完成。 |

`OnStateChanged(from, to)` 是你启动动画或重绘的钩子。

由于 NeoFlux 在 UI 侧单线程，“等待”用 C++20 协程而非线程实现。
`co_await Sleep(duration)` 把协程挂到事件循环的定时器队列上，到时在 UI 线程恢复
——无需回调链，也没有数据竞争：

```cpp
#include <neoflux/core/task.h>

neoflux::Task<void> LongPressDetector(std::weak_ptr<Widget> weak) {
  co_await neoflux::Sleep(std::chrono::milliseconds(500));
  auto w = weak.lock();
  if (!w) co_return;                       // widget 已销毁
  if (w->GetState() == WidgetState::kDragging) {
    // 500ms 后仍按住 -> 确认长按
  }
}
```

::: tip 状态机即条件锁
在按下时启动协程，然后 `Sleep()`。醒来后它**检查 widget 状态**，而不是显式取消什么。
若指针早已提前释放，状态已变，协程静默退出。这正是 `drag_demo` 与
`loading_demo` 使用的模式。完整 `Task` / `Yield` / `Sleep` API 见[协程](./coroutines)。
:::
