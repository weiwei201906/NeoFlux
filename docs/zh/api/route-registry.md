# RouteRegistry / 路由

```cpp
class RouteRegistry : public NonCopyable;
using WidgetBuilder = std::function<std::shared_ptr<Widget>(BuildContext&)>;
```

将路由名字符串映射到 widget 工厂的单例。`Application` 维护导航栈；推入一个路由名即构建
对应 widget 并置于栈顶。

::: tip 类名是 `RouteRegistry`，不是 `Router`
不存在名为 `Router` 的类。注册用 `RouteRegistry`，运行时导航用
`Application::PushRoute` / `Application::PopRoute`。
:::

## 头文件

```cpp
#include <neoflux/widgets/route_registry.h>
```

## 方法

| 方法 | 签名 | 说明 |
|------|------|------|
| `Instance` | `static RouteRegistry& Instance()` | 单例访问器。 |
| `RegisterRoute` | `void RegisterRoute(std::string_view name, WidgetBuilder builder)` | 注册/覆盖路由。 |
| `BuildRoute` | `std::shared_ptr<Widget> BuildRoute(std::string_view name, BuildContext& ctx) const` | 构建路由对应的 widget；未知路由返回 `nullptr`。 |
| `HasRoute` | `bool HasRoute(std::string_view name) const` | 是否存在该路由。 |
| `GetRouteCount` | `std::size_t GetRouteCount() const noexcept` | 已注册路由数。 |
| `Clear` | `void Clear()` | 清空所有路由（测试用）。 |

## 示例

```cpp
#include <neoflux/widgets/route_registry.h>

RouteRegistry::Instance().RegisterRoute("/home", [](BuildContext& ctx) {
    return std::make_shared<Container>()
        ->SetBackgroundColor({255, 255, 255, 255})
        .child(std::make_shared<Text>("Home")->build())
        .build();
});

// 在 widget 内部导航：
//   context.PushRoute("/home");
//   context.PopRoute();
```
