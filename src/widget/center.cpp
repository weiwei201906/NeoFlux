// =============================================================================
// NeoFlux - center.cpp
//
// Implementation of Center. Fills available space (flex-grow=1) and centers
// the child on both axes via Taitank justify/align.
// =============================================================================

#include "neoflux/widget/center.h"

#include <utility>

#include "neoflux/core/types.h"

namespace neoflux {

struct Center::Impl {};

Center::Center(std::shared_ptr<Widget> child)
    : impl_(std::make_unique<Impl>()) {
  SetFlexGrow(1.0F);
  SetJustifyContent(HAlign::kCenter);
  SetAlignItems(VAlign::kCenter);
  if (child != nullptr) {
    SetChild(std::move(child));
  }
}

Center::~Center() = default;

std::string_view Center::GetWidgetName() const noexcept { return "Center"; }

}  // namespace neoflux
