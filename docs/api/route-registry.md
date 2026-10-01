# RouteRegistry / Router

```cpp
class RouteRegistry : public NonCopyable;
using WidgetBuilder = std::function<std::shared_ptr<Widget>(BuildContext&)>;
```

The singleton that maps route name strings to widget factories. The
`Application` keeps a navigation stack; pushing a route name builds the
corresponding widget and places it on top.

::: tip It is named `RouteRegistry`, not `Router`
There is no class called `Router`. Navigation goes through `RouteRegistry`
(registration) and `Application::PushRoute` / `Application::PopRoute` (runtime).
:::

## Header

```cpp
#include <neoflux/widgets/route_registry.h>
```

## Methods

| Method | Signature | Notes |
|--------|-----------|-------|
| `Instance` | `static RouteRegistry& Instance()` | Singleton accessor. |
| `RegisterRoute` | `void RegisterRoute(std::string_view name, WidgetBuilder builder)` | Register/overwrite a route. |
| `BuildRoute` | `std::shared_ptr<Widget> BuildRoute(std::string_view name, BuildContext& ctx) const` | Build a route's widget; `nullptr` if unknown. |
| `HasRoute` | `bool HasRoute(std::string_view name) const` | Existence check. |
| `GetRouteCount` | `std::size_t GetRouteCount() const noexcept` | Registered route count. |
| `Clear` | `void Clear()` | Remove all routes (testing). |

## Example

```cpp
#include <neoflux/widgets/route_registry.h>

RouteRegistry::Instance().RegisterRoute("/home", [](BuildContext& ctx) {
    return std::make_shared<Container>()
        ->SetBackgroundColor({255, 255, 255, 255})
        .child(std::make_shared<Text>("Home")->build())
        .build();
});

// Navigate from inside a widget:
//   context.PushRoute("/home");
//   context.PopRoute();
```
