// =============================================================================
// NeoFlux - widgets/back_button.cpp
//
// Implementation of BackButton: wraps a Button whose press callback pops the
// navigation stack. The Application is fetched via BuildContext so the widget
// needs no external wiring.
// =============================================================================

#include "widgets/back_button.h"

#include <neoflux/app/application.h>
#include <neoflux/widget/button.h>

#include <memory>

namespace neoflux {

BackButton::BackButton(BuildContext& /*context*/) {}

void BackButton::GoBack() {
  Application* app = GetApplication();
  if (app != nullptr) {
    app->PopRoute();
  }
}

std::shared_ptr<Widget> BackButton::Build(BuildContext& /*context*/) {
  auto button = std::make_shared<Button>("Back");
  button->SetOnPressed([this]() { GoBack(); });
  return button;
}

}  // namespace neoflux
