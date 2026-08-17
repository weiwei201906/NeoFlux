# Routing

NeoFlux provides a simple route-based navigation system. Pages are registered
with a route name and a builder function, then pushed onto a navigation stack.

## Registering Routes

Use `RouteRegistry` to map route names to builder functions:

```cpp
#include <neoflux/widget/route_registry.h>

std::shared_ptr<Widget> BuildHomePage(BuildContext& context) {
  auto root = std::make_shared<Container>();
  // ... build home page
  return root;
}

std::shared_ptr<Widget> BuildSettingsPage(BuildContext& context) {
  auto root = std::make_shared<Container>();
  // ... build settings page
  return root;
}

// Register routes (typically in main before app.Run())
RouteRegistry::Instance().RegisterRoute("/", BuildHomePage);
RouteRegistry::Instance().RegisterRoute("/settings", BuildSettingsPage);
```

:::tip
Even if your project has only one route, you must still register it with
`RouteRegistry` and then call `app.PushRoute("/")` to display it.
`Application::Init` does not automatically push any route — an empty
navigation stack renders nothing.
:::

## Navigation

### Push a Route

```cpp
app.PushRoute("/settings");
```

This builds the new page's widget tree and displays it. The previous page
remains on the stack.

### Pop a Route

```cpp
app.PopRoute();
```

Returns to the previous route on the stack.

### Replace Current Route

```cpp
app.ReplaceRoute("/login");
```

Replaces the current route without adding to the stack.

## Navigation from Within a Widget

Widgets can trigger navigation by holding a reference to the `Application`
or using a callback:

```cpp
class HomePage : public StatefulWidget {
 public:
  explicit HomePage(Application& app) : app_(app) {}

  std::shared_ptr<Widget> Build(BuildContext&) override {
    auto btn = std::make_shared<Button>("Go to Settings");
    btn->SetOnPressed([this]() {
      app_.get().PushRoute("/settings");
    });
    return btn;
  }

 private:
  std::reference_wrapper<Application> app_;
};
```

## Route Builder Signature

Route builder functions receive a `BuildContext` and return a
`std::shared_ptr<Widget>`:

```cpp
using RouteBuilder = std::function<std::shared_ptr<Widget>(BuildContext&)>;
```

`BuildContext` provides access to the `Application` instance for navigation and
state.

## Example: Multi-Page App

```cpp
int main(int argc, char** argv) {
  RouteRegistry::Instance().RegisterRoute("/", BuildHome);
  RouteRegistry::Instance().RegisterRoute("/about", BuildAbout);
  RouteRegistry::Instance().RegisterRoute("/settings", BuildSettings);

  Application app;
  app.Init(argc, argv, 400, 600, "Multi-Page App");
  app.PushRoute("/");
  app.Run();
  return 0;
}
```

Each page can contain buttons that push other routes, creating a navigation
hierarchy.
