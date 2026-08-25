// =============================================================================
// NeoFlux - Counter example
//
// A minimal counter application demonstrating:
//   - StatefulWidget with mutable state
//   - Button press callbacks
//   - Taitank flex layout (row of buttons)
//   - Route registration and event loop
// =============================================================================

#include <neoflux/app/application.h>
#include <neoflux/widget/button.h>
#include <neoflux/widget/container.h>
#include <neoflux/widget/route_registry.h>
#include <neoflux/widget/text.h>
#include <neoflux/widget/widget.h>

#include <memory>
#include <string>

namespace neoflux {
namespace {

// A stateful counter widget. Holds an integer count and rebuilds when it
// changes via SetState().
class CounterWidget : public StatefulWidget {
 public:
  [[nodiscard]] std::string_view GetWidgetName() const noexcept override {
    return "CounterWidget";
  }

  [[nodiscard]] std::unique_ptr<State<StatefulWidget>> CreateState() override;
};

class CounterState : public State<StatefulWidget> {
 public:
  [[nodiscard]] std::shared_ptr<Widget> Build(BuildContext& /*context*/) override {
    auto col = std::make_shared<Container>();
    col->SetFlexDirection(FlexDirection::kColumn)
        .SetJustifyContent(HAlign::kCenter)
        .SetAlignItems(VAlign::kCenter)
        .SetPadding({.left = 40.0F, .top = 40.0F, .right = 40.0F, .bottom = 40.0F});

    auto count_text = std::make_shared<Text>("Count: " + std::to_string(count_));
    count_text->SetFontSize(48.0F)
        .SetTextColor({.r = 33, .g = 33, .b = 33, .a = 255})
        .SetAlignment(HAlign::kCenter);

    // Row container for the two buttons.
    auto button_row = std::make_shared<Container>();
    button_row->SetFlexDirection(FlexDirection::kRow)
        .SetJustifyContent(HAlign::kCenter)
        .SetAlignItems(VAlign::kCenter)
        .SetMargin({.top = 24.0F});

    auto decrement = std::make_shared<Button>("-");
    decrement->SetFontSize(24.0F).SetOnPressed([this] {
      SetState([this] { --count_; });
    });

    auto increment = std::make_shared<Button>("+");
    increment->SetFontSize(24.0F).SetOnPressed([this] {
      SetState([this] { ++count_; });
    });

    button_row->AddChild(decrement);
    button_row->AddChild(increment);

    col->AddChild(count_text);
    col->AddChild(button_row);
    return col;
  }

 private:
  int count_ = 0;
};

std::unique_ptr<State<StatefulWidget>> CounterWidget::CreateState() {
  return std::make_unique<CounterState>();
}

// Root page: a full-window Container that centers the CounterWidget.
std::shared_ptr<Widget> BuildCounterPage(BuildContext& /*context*/) {
  auto root = std::make_shared<Container>();
  root->SetFlexDirection(FlexDirection::kColumn)
      .SetJustifyContent(HAlign::kCenter)
      .SetAlignItems(VAlign::kCenter)
      .SetBackgroundColor({.r = 255, .g = 255, .b = 255, .a = 255});
  root->AddChild(std::make_shared<CounterWidget>());
  return root;
}

}  // namespace
}  // namespace neoflux

int main(int argc, char** argv) {
  using namespace neoflux;

  RouteRegistry::Instance().RegisterRoute("/", BuildCounterPage);

  Application app;
  app.SetFontDir("./fonts/");
  if (!app.Init(argc, argv, 400, 300, "NeoFlux Counter")) {
    return 1;
  }
  app.PushRoute("/");
  app.Run();
  return 0;
}
