// =============================================================================
// NeoFlux - views/counter_view.cpp
//
// Counter demo implementation. The state object owns the count; SetState
// marks the owning widget dirty so the framework rebuilds it next frame.
// =============================================================================

#include "views/counter_view.h"

#include <neoflux/widget/button.h>
#include <neoflux/widget/container.h>
#include <neoflux/widget/sized_box.h>
#include <neoflux/widget/text.h>

#include "widgets/back_button.h"

#include <memory>
#include <string>
#include <utility>

namespace neoflux {

CounterView::CounterView(BuildContext& /*context*/) {}

std::unique_ptr<State<StatefulWidget>> CounterView::CreateState() {
  return std::make_unique<CounterViewState>();
}

void CounterViewState::Increment() {
  SetState([this]() { ++count_; });
}

std::shared_ptr<Widget> CounterViewState::Build(BuildContext& context) {
  auto root = std::make_shared<Container>();
  root->SetBackgroundColor({.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF})
      .SetPadding({.left = 24.0F, .top = 24.0F, .right = 24.0F,
                   .bottom = 24.0F});

  auto title = std::make_shared<Text>("Counter");
  title->SetFontSize(26.0F)
      .SetTextColor({.r = 0x1A, .g = 0x1A, .b = 0x1A, .a = 0xFF});

  const std::string count_text = "Count: " + std::to_string(count_);
  auto value = std::make_shared<Text>(count_text);
  value->SetFontSize(20.0F)
      .SetTextColor({.r = 0x33, .g = 0x33, .b = 0x33, .a = 0xFF});

  auto increment = std::make_shared<Button>("Increment");
  increment->SetOnPressed([this]() { Increment(); });

  // Shared reusable widget from src/widgets/: pops the navigation stack.
  auto back = std::make_shared<BackButton>(context);

  root->AddChild(title);
  root->AddChild(std::make_shared<SizedBox>(0.0F, 16.0F));
  root->AddChild(value);
  root->AddChild(std::make_shared<SizedBox>(0.0F, 24.0F));
  root->AddChild(increment);
  root->AddChild(std::make_shared<SizedBox>(0.0F, 12.0F));
  root->AddChild(back);
  return root;
}

}  // namespace neoflux
