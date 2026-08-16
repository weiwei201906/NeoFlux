// =============================================================================
// NeoFlux - Hello World example
//
// Demonstrates the minimal usage of the NeoFlux framework:
//   1. Register routes (widgets) with the RouteRegistry.
//   2. Create an Application.
//   3. Initialize and run the event loop.
//
// This example creates a simple UI with a container, text, and a button
// that navigates between two routes.
// =============================================================================

#include <neoflux/neoflux.h>

#include <memory>

#include <glog/logging.h>

namespace neoflux {
namespace {

// A custom stateful widget that demonstrates the Flutter-like pattern.
class CounterWidget : public StatefulWidget {
 public:
  [[nodiscard]] std::string_view GetWidgetName() const noexcept override {
    return "CounterWidget";
  }

  [[nodiscard]] std::unique_ptr<State<StatefulWidget>> CreateState() override;
};

// State for the CounterWidget.
class CounterState : public State<StatefulWidget> {
 public:
  [[nodiscard]] std::shared_ptr<Widget> Build(BuildContext& context) override {
    auto container = std::make_shared<Container>();
    container->SetBackgroundColor({240, 240, 240, 255})
        .SetPadding({20.0F, 20.0F, 20.0F, 20.0F});

    auto text = std::make_shared<Text>("Count: " + std::to_string(count_));
    text->SetFontSize(24.0F).SetTextColor({33, 33, 33, 255});

    auto button = std::make_shared<Button>("Increment");
    button->SetOnPressed([this]() {
      SetState([this]() { ++count_; });
    });

    container->AddChild(text);
    container->AddChild(button);
    return container;
  }

 private:
  int count_ = 0;
};

std::unique_ptr<State<StatefulWidget>> CounterWidget::CreateState() {
  return std::make_unique<CounterState>();
}

// Builds the home page widget.
std::shared_ptr<Widget> BuildHomePage(BuildContext& context) {
  auto root = std::make_shared<Container>();
  root->SetBackgroundColor({255, 255, 255, 255})
      .SetPadding({40.0F, 40.0F, 40.0F, 40.0F});

  auto title = std::make_shared<Text>("NeoFlux Hello World");
  title->SetFontSize(32.0F).SetTextColor({0, 0, 0, 255});

  auto subtitle = std::make_shared<Text>(
      "A cross-platform C++20 UI framework");
  subtitle->SetFontSize(16.0F).SetTextColor({100, 100, 100, 255});

  auto counter = std::make_shared<CounterWidget>();

  auto nav_button = std::make_shared<Button>("Go to About Page");
  nav_button->SetOnPressed([&context]() { context.PushRoute("/about"); });

  root->AddChild(title);
  root->AddChild(subtitle);
  root->AddChild(counter);
  root->AddChild(nav_button);

  return root;
}

// Builds the about page widget.
std::shared_ptr<Widget> BuildAboutPage(BuildContext& context) {
  auto root = std::make_shared<Container>();
  root->SetBackgroundColor({255, 255, 255, 255})
      .SetPadding({40.0F, 40.0F, 40.0F, 40.0F});

  auto title = std::make_shared<Text>("About NeoFlux");
  title->SetFontSize(28.0F).SetTextColor({0, 0, 0, 255});

  auto desc = std::make_shared<Text>(
      "Two-layer architecture: Application + Render. "
      "SPSC ring queue for thread-safe communication. "
      "tgfx for mobile rendering, GLFW bridge for desktop.");
  desc->SetFontSize(14.0F).SetTextColor({60, 60, 60, 255});

  auto back_button = std::make_shared<Button>("Back");
  back_button->SetOnPressed([&context]() { context.PopRoute(); });

  root->AddChild(title);
  root->AddChild(desc);
  root->AddChild(back_button);

  return root;
}

}  // namespace
}  // namespace neoflux

int main(int argc, char** argv) {
  using namespace neoflux;

  // Register routes.
  RouteRegistry::Instance().RegisterRoute("/", BuildHomePage);
  RouteRegistry::Instance().RegisterRoute("/about", BuildAboutPage);

  // Create and run the application.
  Application app;
  if (!app.Init(argc, argv, 800, 600, "NeoFlux - Hello World")) {
    LOG(ERROR) << "Failed to initialize application";
    return 1;
  }

  // Push the initial route.
  app.PushRoute("/");

  // Run the event loop (blocks until window close or Stop()).
  app.Run();

  return 0;
}
