// =============================================================================
// NeoFlux - padding.cpp
//
// Implementation of Padding. Delegates all layout to Taitank via the Container
// base class; only the edge insets are configured here.
// =============================================================================

#include "neoflux/widget/padding.h"

#include <utility>

namespace neoflux {

Padding::Padding(float all, std::shared_ptr<Widget> child) {
  SetPadding({.left = all, .top = all, .right = all, .bottom = all});
  if (child != nullptr) {
    SetChild(std::move(child));
  }
}

Padding::Padding(const EdgeInsets& insets, std::shared_ptr<Widget> child) {
  SetPadding(insets);
  if (child != nullptr) {
    SetChild(std::move(child));
  }
}

std::string_view Padding::GetWidgetName() const noexcept { return "Padding"; }

}  // namespace neoflux
