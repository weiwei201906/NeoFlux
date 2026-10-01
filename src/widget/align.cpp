// =============================================================================
// NeoFlux - align.cpp
//
// Implementation of Align. Fills available space (flex-grow=1) and positions
// the child at the requested alignment. With the default column flex
// direction, justify-content governs the vertical (main) axis and align-items
// governs the horizontal (cross) axis.
// =============================================================================

#include "neoflux/widget/align.h"

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

Align::Align(HAlign horizontal, VAlign vertical,
             std::shared_ptr<Widget> child)
    : horizontal_(horizontal), vertical_(vertical) {
  SetFlexGrow(1.0F);
  ApplyAlignment();
  if (child != nullptr) {
    SetChild(std::move(child));
  }
}

std::string_view Align::GetWidgetName() const noexcept { return "Align"; }

Align& Align::SetHorizontal(HAlign horizontal) noexcept {
  horizontal_ = horizontal;
  ApplyAlignment();
  return *this;
}

Align& Align::SetVertical(VAlign vertical) noexcept {
  vertical_ = vertical;
  ApplyAlignment();
  return *this;
}

void Align::ApplyAlignment() noexcept {
  auto* node = GetTaitankNode();
  if (node == nullptr) {
    return;
  }
  taitank::SetAlignItems(node, CrossAlign(horizontal_));
  taitank::SetJustifyContent(node, MainJustify(vertical_));
}

}  // namespace neoflux
