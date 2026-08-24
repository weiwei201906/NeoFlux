// =============================================================================
// NeoFlux - TextField example
//
// Demonstrates the TextField widget:
//   - Text input with cursor navigation
//   - Placeholder text
//   - OnSubmit callback (Enter key)
//   - OnChange callback (every keystroke)
//   - Keyboard focus management
//   - Taitank flex layout
// =============================================================================

#include <neoflux/app/application.h>
#include <neoflux/widget/button.h>
#include <neoflux/widget/container.h>
#include <neoflux/widget/route_registry.h>
#include <neoflux/widget/text.h>
#include <neoflux/widget/text_field.h>
#include <neoflux/widget/widget.h>

#include <memory>
#include <string>

namespace neoflux {
namespace {

class FormWidget : public StatefulWidget {
 public:
  [[nodiscard]] std::string_view GetWidgetName() const noexcept override {
    return "FormWidget";
  }

  [[nodiscard]] std::unique_ptr<State<StatefulWidget>> CreateState() override;
};

class FormState : public State<StatefulWidget> {
 public:
  [[nodiscard]] std::shared_ptr<Widget> Build(BuildContext& /*context*/) override {
    auto col = std::make_shared<Container>();
    col->SetFlexDirection(FlexDirection::kColumn)
        .SetJustifyContent(HAlign::kCenter)
        .SetAlignItems(VAlign::kCenter)
        .SetPadding({.left = 40.0F, .top = 40.0F, .right = 40.0F, .bottom = 40.0F})
        .SetBackgroundColor({.r = 245, .g = 245, .b = 250, .a = 255});

    auto title = std::make_shared<Text>("NeoFlux Form");
    title->SetFontSize(28.0F)
        .SetTextColor({.r = 30, .g = 30, .b = 60, .a = 255});

    auto name_label = std::make_shared<Text>("Name:");
    name_label->SetFontSize(14.0F)
        .SetTextColor({.r = 80, .g = 80, .b = 80, .a = 255});

    auto name_field = std::make_shared<TextField>();
    name_field->SetPlaceholder("Enter your name...");
    name_field->SetFontSize(16.0F);
    name_field->SetOnChange([this](std::string_view text) {
      name_ = std::string(text);
    });

    auto email_label = std::make_shared<Text>("Email:");
    email_label->SetFontSize(14.0F)
        .SetTextColor({.r = 80, .g = 80, .b = 80, .a = 255});

    auto email_field = std::make_shared<TextField>();
    email_field->SetPlaceholder("Enter your email...");
    email_field->SetFontSize(16.0F);
    email_field->SetOnChange([this](std::string_view text) {
      email_ = std::string(text);
    });
    email_field->SetOnSubmit([this](std::string_view /*text*/) {
      Submit();
    });

    auto submit_btn = std::make_shared<Button>("Submit");
    submit_btn->SetFontSize(16.0F)
        .SetBackgroundColor({.r = 66, .g = 133, .b = 244, .a = 255})
        .SetTextColor({.r = 255, .g = 255, .b = 255, .a = 255})
        .SetOnPressed([this] { Submit(); });

    result_text_ = std::make_shared<Text>(result_message_);
    result_text_->SetFontSize(14.0F)
        .SetTextColor({.r = 60, .g = 140, .b = 60, .a = 255});

    // Wrap each widget in a Container for spacing.
    auto wrap = [](std::shared_ptr<Widget> child, float margin_bottom) {
      auto c = std::make_shared<Container>();
      c->SetMargin({.bottom = margin_bottom});
      c->SetChild(child);
      return c;
    };

    col->AddChild(wrap(title, 24.0F));
    col->AddChild(wrap(name_label, 4.0F));
    col->AddChild(wrap(name_field, 16.0F));
    col->AddChild(wrap(email_label, 4.0F));
    col->AddChild(wrap(email_field, 24.0F));
    col->AddChild(wrap(submit_btn, 20.0F));
    col->AddChild(result_text_);
    return col;
  }

 private:
  void Submit() {
    if (name_.empty() && email_.empty()) {
      result_message_ = "Please fill in at least one field.";
    } else {
      result_message_ = "Submitted: " + name_ + " / " + email_;
    }
    if (result_text_ != nullptr) {
      result_text_->SetText(result_message_);
    }
    SetState([] {});
  }

  std::string name_{};
  std::string email_{};
  std::string result_message_{"Fill in the form and press Submit."};
  std::shared_ptr<Text> result_text_{};
};

std::unique_ptr<State<StatefulWidget>> FormWidget::CreateState() {
  return std::make_unique<FormState>();
}

std::shared_ptr<Widget> BuildFormPage(BuildContext& /*context*/) {
  auto root = std::make_shared<Container>();
  root->SetFlexDirection(FlexDirection::kColumn)
      .SetJustifyContent(HAlign::kCenter)
      .SetAlignItems(VAlign::kCenter)
      .SetBackgroundColor({.r = 245, .g = 245, .b = 250, .a = 255});
  root->AddChild(std::make_shared<FormWidget>());
  return root;
}

}  // namespace
}  // namespace neoflux

int main(int argc, char** argv) {
  using namespace neoflux;

  RouteRegistry::Instance().RegisterRoute("/", BuildFormPage);

  Application app;
  app.SetFontDir("thirdparty/fonts");
  if (!app.Init(argc, argv, 420, 480, "NeoFlux TextField Demo")) {
    return 1;
  }
  app.PushRoute("/");
  app.Run();
  return 0;
}
