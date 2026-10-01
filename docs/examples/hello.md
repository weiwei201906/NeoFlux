# Hello NeoFlux

The `hello_neoflux` example wires together everything a real NeoFlux app uses:
two registered routes, a stateful counter widget, button callbacks, and
flex layout.

## Run

From the repository root after building with MSVC + Ninja:

```powershell
.\build\bin\hello_neoflux.exe
```

::: tip
Want to see logs? The binary is GUI-subsystem (no console). Run with:

```powershell
.\build\bin\hello_neoflux.exe --logtostderr --verbose_logging
```
:::

## What you will see

- A title, a subtitle, and a live counter.
- An **Increment** button that bumps the count.
- A **Go to About Page** button that pushes a second route.
- A **Back** button on the About page that pops the route.

## How it maps to the framework

| What happens | Code |
|--------------|------|
| Two routes registered | `RouteRegistry::Instance().RegisterRoute("/", ...)` and `("/about", ...)` |
| A mutable counter | `CounterWidget : StatefulWidget` with a `CounterState` |
| Rebuild on tap | `SetState([this]{ ++count_; });` inside the button callback |
| Navigate forward | `app->PushRoute("/about")` |
| Navigate back | `app->PopRoute()` |

## Key code

The stateful counter (`examples/hello_neoflux/main.cpp`):

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

And the entry point:

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
