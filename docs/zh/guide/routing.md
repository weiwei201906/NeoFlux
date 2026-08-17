# 路由导航

Widget 通过 `RouteRegistry` 注册，压入/弹出导航栈。

## 注册路由

```cpp
RouteRegistry::Instance().RegisterRoute("/home", BuildHome);
RouteRegistry::Instance().RegisterRoute("/settings", BuildSettingsPage);
```

:::tip
哪怕你的项目只有一个路由，也必须先通过 `RouteRegistry` 注册，再调用 `app.PushRoute("/")` 才能显示。`Application::Init` 不会自动推送任何路由——空的导航栈不会渲染任何内容。
:::

## 导航

```cpp
app.PushRoute("/settings");  // 构建并显示设置页面
app.PopRoute();              // 返回上一路由
```

## 路由构建函数

每个路由对应一个 `WidgetBuilder` 函数，接收 `BuildContext`，返回 `std::shared_ptr<Widget>`：

```cpp
std::shared_ptr<Widget> BuildHome(BuildContext& ctx) {
  auto root = std::make_shared<Container>();
  // ...
  return root;
}
```

## 下一步

- [Widget 系统](./widgets)
- [协程](./coroutines)
