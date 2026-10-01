// =============================================================================
// NeoFlux - divider.cpp
//
// Implementation of Divider. The line is painted by the Container base class
// (solid background rect); this file defines Divider::Impl, which holds the
// thickness/orientation state and applies the fixed thickness to Taitank.
// =============================================================================

#include "neoflux/widget/divider.h"

#include <cmath>

#include "taitank.h"

namespace neoflux {

namespace {
// Default separator color: neutral light gray.
constexpr Color kDefaultColor{.r = 0xE0, .g = 0xE0, .b = 0xE0, .a = 0xFF};
}  // namespace

struct Divider::Impl {
  float thickness = 1.0F;
  DividerOrientation orientation = DividerOrientation::kHorizontal;

  // Pins the fixed thickness on the axis perpendicular to the line and leaves
  // the parallel axis auto so the divider stretches to fill its parent.
  void ApplyTo(taitank::TaitankNode* node) const;
};

void Divider::Impl::ApplyTo(taitank::TaitankNode* node) const {
  if (node == nullptr) {
    return;
  }
  taitank::SetWidth(node, NAN);
  taitank::SetHeight(node, NAN);
  if (orientation == DividerOrientation::kHorizontal) {
    taitank::SetHeight(node, thickness);
  } else {
    taitank::SetWidth(node, thickness);
  }
}

Divider::Divider() : impl_(std::make_unique<Impl>()) {
  SetBackgroundColor(kDefaultColor);
  impl_->ApplyTo(GetTaitankNode());
}

Divider::Divider(float thickness) : impl_(std::make_unique<Impl>()) {
  impl_->thickness = thickness > 0.0F ? thickness : 1.0F;
  SetBackgroundColor(kDefaultColor);
  impl_->ApplyTo(GetTaitankNode());
}

Divider::~Divider() = default;

std::string_view Divider::GetWidgetName() const noexcept { return "Divider"; }

Divider& Divider::SetColor(const Color& color) noexcept {
  SetBackgroundColor(color);
  return *this;
}

Divider& Divider::SetThickness(float thickness) noexcept {
  impl_->thickness = thickness > 0.0F ? thickness : 1.0F;
  impl_->ApplyTo(GetTaitankNode());
  return *this;
}

Divider& Divider::SetOrientation(DividerOrientation orientation) noexcept {
  impl_->orientation = orientation;
  impl_->ApplyTo(GetTaitankNode());
  return *this;
}

}  // namespace neoflux
