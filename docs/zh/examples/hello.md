# Hello NeoFlux

`hello_neoflux` 示例把一个真实 NeoFlux 应用用到的所有东西串了起来：
两个已注册路由、一个有状态计数器 widget、按钮回调与 flex 布局。

## 运行

用 MSVC + Ninja 构建后，从仓库根目录运行：

```powershell
.\build\bin\hello_neoflux.exe
```

::: tip
想看日志？该二进制是 GUI 子系统（无控制台）。运行时加：

```powershell
.\build\bin\hello_neoflux.exe --logtostderr --verbose_logging
```
:::

## 你会看到

- 一个标题、一段副标题和实时计数器。
- **Increment** 按钮让计数加一。
- **Go to About Page** 按钮压入第二个路由。
- About 页上的 **Back** 按钮弹出该路由。

## 与框架的对应关系

| 发生的事 | 代码 |
|----------|------|
| 注册两条路由 | `RouteRegistry::Instance().RegisterRoute("/", ...)` 与 `("/about", ...)` |
| 可变计数器 | `CounterWidget : StatefulWidget` 及其 `CounterState` |
| 点击后重建 | 按钮回调里 `SetState([this]{ ++count_; });` |
| 前进导航 | `app->PushRoute("/about")` |
| 返回导航 | `app->PopRoute()` |

## 关键代码

有状态计数器（`examples/hello_neoflux/main.cpp`）：

```cpp
class CounterState : public State<StatefulWidget> {
 public:
  std::shared_ptr<Widget> Build(BuildContext&) override {
    auto container = std::make_shared<Container>();
    container->SetBackgroundColor({.r=240, .g=240, .b=245, .a=255})
        .SetPadding({.left=20, .top=20, .right=20, .bottom=20});

    auto text = std::make_shared<Text>("Count: " + std::to_string(count_));
    text->SetFontSize(24.0F).SetTextColor({.r=33, .g=33, .b=33, .a=255});

    const auto button = std::make_shared<Button>("Increment");
    button->SetOnPressed([this] { SetState([this] { ++count_; }); });

    container->AddChild(text);
    container->AddChild(button);
    return container;
  }
 private:
  int count_ = 0;
};
```

入口：

```cpp
int main(int argc, char** argv) {
  RouteRegistry::Instance().RegisterRoute("/", BuildHomePage);
  RouteRegistry::Instance().RegisterRoute("/about", BuildAboutPage);

  Application app;
  if (!app.Init(argc, argv, 800, 600, "NeoFlux - Hello World")) return 1;
  app.PushRoute("/");
  app.Run();
  return 0;
}
```
