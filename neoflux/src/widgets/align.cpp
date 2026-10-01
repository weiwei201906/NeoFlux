// =============================================================================
// NeoFlux - align.cpp
//
// Implementation of Align. Fills available space (flex-grow=1) and positions
// the child at the requested alignment. With the default column flex
// direction, justify-content governs the vertical (main) axis and align-items
// governs the horizontal (cross) axis.
// =============================================================================

#include "neoflux/widgets/align.h"

#include <utility>

#include "taitank.h"

#include "neoflux/core/types.h"

namespace neoflux {

namespace {

// Maps a horizontal position to the Taitank cross-axis alignment.
taitank::FlexAlign CrossAlign(HAlign horizontal) {
  switch (horizontal) {
    case HAlign::kLeft:
      return taitank::FLEX_ALIGN_START;
    case HAlign::kRight:
      return taitank::FLEX_ALIGN_END;
    case HAlign::kCenter:
    default:
      return taitank::FLEX_ALIGN_CENTER;
  }
}

// Maps a vertical position to the Taitank main-axis justification.
taitank::FlexAlign MainJustify(VAlign vertical) {
  switch (vertical) {
    case VAlign::kTop:
      return taitank::FLEX_ALIGN_START;
    case VAlign::kBottom:
      return taitank::FLEX_ALIGN_END;
    case VAlign::kCenter:
    default:
      return taitank::FLEX_ALIGN_CENTER;
  }
}

}  // namespace

struct Align::Impl {
  HAlign horizontal = HAlign::kCenter;
  VAlign vertical = VAlign::kCenter;

  // Pushes the current alignment onto the Taitank node.
  void ApplyTo(taitank::TaitankNode* node) const;
};

void Align::Impl::ApplyTo(taitank::TaitankNode* node) const {
  if (node == nullptr) {
    return;
  }
  taitank::SetAlignItems(node, CrossAlign(horizontal));
  taitank::SetJustifyContent(node, MainJustify(vertical));
}

Align::Align(HAlign horizontal, VAlign vertical,
             std::shared_ptr<Widget> child)
    : impl_(std::make_unique<Impl>()) {
  impl_->horizontal = horizontal;
  impl_->vertical = vertical;
  SetFlexGrow(1.0F);
  impl_->ApplyTo(GetTaitankNode());
  if (child != nullptr) {
    SetChild(std::move(child));
  }
}

Align::~Align() = default;

std::string_view Align::GetWidgetName() const noexcept { return "Align"; }

Align& Align::SetHorizontal(HAlign horizontal) noexcept {
  impl_->horizontal = horizontal;
  impl_->ApplyTo(GetTaitankNode());
  return *this;
}

Align& Align::SetVertical(VAlign vertical) noexcept {
  impl_->vertical = vertical;
  impl_->ApplyTo(GetTaitankNode());
  return *this;
}

}  // namespace neoflux
