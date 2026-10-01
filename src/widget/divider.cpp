// =============================================================================
// NeoFlux - divider.cpp
//
// Implementation of Divider. The line is painted by the Container base class
// (solid background rect); this file only configures the fixed thickness on
// the correct axis and picks a default separator color.
// =============================================================================

#include "neoflux/widget/divider.h"

namespace neoflux {

namespace {
// Default separator color: neutral light gray.
constexpr Color kDefaultColor{.r = 0xE0, .g = 0xE0, .b = 0xE0, .a = 0xFF};
}  // namespace

Divider::Divider() {
  SetBackgroundColor(kDefaultColor);
  ApplyThickness();
}

Divider::Divider(float thickness) : thickness_(thickness > 0.0F ? thickness : 1.0F) {
  SetBackgroundColor(kDefaultColor);
  ApplyThickness();
}

std::string_view Divider::GetWidgetName() const noexcept { return "Divider"; }

Divider& Divider::SetColor(const Color& color) noexcept {
  SetBackgroundColor(color);
  return *this;
}

Divider& Divider::SetThickness(float thickness) noexcept {
  thickness_ = thickness > 0.0F ? thickness : 1.0F;
  ApplyThickness();
  return *this;
}

Divider& Divider::SetOrientation(DividerOrientation orientation) noexcept {
  orientation_ = orientation;
  ApplyThickness();
  return *this;
}

void Divider::ApplyThickness() noexcept {
  // Clear any previously set fixed dimension, then pin thickness on the axis
  // perpendicular to the line. The parallel axis stays auto so the divider
  // stretches to fill its parent (align-items: stretch by default).
  Container::SetWidth(0.0F);
  Container::SetHeight(0.0F);
  if (orientation_ == DividerOrientation::kHorizontal) {
    Container::SetHeight(thickness_);
  } else {
    Container::SetWidth(thickness_);
  }
}

}  // namespace neoflux
