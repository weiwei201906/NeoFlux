// =============================================================================
// NeoFlux - views/home_view.cpp
//
// Landing page implementation. Demonstrates a scrollable column of buttons
// that navigate to the other demo routes.
// =============================================================================

#include "views/home_view.h"

#include <neoflux/app/application.h>
#include <neoflux/widget/button.h>
#include <neoflux/widget/container.h>
#include <neoflux/widget/scroll_view.h>
#include <neoflux/widget/sized_box.h>
#include <neoflux/widget/text.h>

#include <memory>
#include <string>
#include <string_view>

namespace neoflux {

HomeView::HomeView(BuildContext& /*context*/) {}

void HomeView::NavigateTo(std::string_view route) {
  // PushRoute replaces the current page with the target route. The
  // Application outlives every widget, so capturing it in callbacks is safe.
  Application* app = GetApplication();
  if (app != nullptr) {
    app->PushRoute(route);
  }
}

std::shared_ptr<Widget> HomeView::Build(BuildContext& /*context*/) {
  auto root = std::make_shared<Container>();
  root->SetBackgroundColor({.r = 0xF7, .g = 0xF8, .b = 0xFA, .a = 0xFF})
      .SetPadding({.left = 24.0F, .top = 24.0F, .right = 24.0F,
                   .bottom = 24.0F});

  auto title = std::make_shared<Text>("NeoFlux Quick Start");
  title->SetFontSize(28.0F)
      .SetTextColor({.r = 0x1A, .g = 0x1A, .b = 0x1A, .a = 0xFF});

  auto hint = std::make_shared<Text>(
      "A cross-platform C++20 UI framework. Pick a demo below.");
  hint->SetFontSize(14.0F)
      .SetTextColor({.r = 0x8A, .g = 0x8A, .b = 0x8A, .a = 0xFF});

  auto counter_button = std::make_shared<Button>("Counter Demo");
  counter_button->SetOnPressed([this]() { NavigateTo("/counter"); });

  auto about_button = std::make_shared<Button>("About");
  about_button->SetOnPressed([this]() { NavigateTo("/about"); });

  root->AddChild(title);
  root->AddChild(std::make_shared<SizedBox>(0.0F, 8.0F));
  root->AddChild(hint);
  root->AddChild(std::make_shared<SizedBox>(0.0F, 24.0F));
  root->AddChild(counter_button);
  root->AddChild(std::make_shared<SizedBox>(0.0F, 12.0F));
  root->AddChild(about_button);
  return root;
}

}  // namespace neoflux
