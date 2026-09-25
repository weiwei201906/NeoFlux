// =============================================================================
// NeoFlux - container.cpp
//
// Implementation of Container widget. All layout is delegated to Taitank;
// this file only configures Taitank style properties and paints the
// background.
// =============================================================================

#include "neoflux/widget/container.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string_view>

#include <glog/logging.h>

#include "taitank.h"

#include "neoflux/core/types.h"
#include "neoflux/render/render_context.h"
#include "neoflux/widget/widget.h"

namespace neoflux {

namespace {

taitank::FlexDirection ToTaitankDirection(FlexDirection direction) {
  switch (direction) {
    case FlexDirection::kRow:
      return taitank::FLEX_DIRECTION_ROW;
    case FlexDirection::kRowReverse:
      return taitank::FLEX_DIRECTION_ROW_REVERSE;
    case FlexDirection::kColumnReverse:
      return taitank::FLEX_DIRECTION_COLUNM_REVERSE;
    case FlexDirection::kColumn:
    default:
      return taitank::FLEX_DIRECTION_COLUMN;
  }
}

taitank::FlexAlign ToTaitankAlign(HAlign align) {
  switch (align) {
    case HAlign::kCenter:
      return taitank::FLEX_ALIGN_CENTER;
    case HAlign::kRight:
      return taitank::FLEX_ALIGN_END;
    case HAlign::kLeft:
    default:
      return taitank::FLEX_ALIGN_START;
  }
}

taitank::FlexAlign ToTaitankVAlign(VAlign align) {
  switch (align) {
    case VAlign::kCenter:
      return taitank::FLEX_ALIGN_CENTER;
    case VAlign::kBottom:
      return taitank::FLEX_ALIGN_END;
    case VAlign::kTop:
    default:
      return taitank::FLEX_ALIGN_START;
  }
}

}  // namespace

Container::Container() {
  auto* node = GetTaitankNode();
  if (node != nullptr) {
    taitank::SetFlexDirection(node, taitank::FLEX_DIRECTION_COLUMN);
    taitank::SetAlignItems(node, taitank::FLEX_ALIGN_STRETCH);
  }
}

Container::~Container() = default;

std::string_view Container::GetWidgetName() const noexcept {
  return "Container";
}

Container& Container::SetBackgroundColor(const Color& color) noexcept {
  background_color_ = color;
  has_background_ = true;
  return *this;
}

Container& Container::SetPadding(const EdgeInsets& padding) noexcept {
  padding_ = padding;
  ApplyPaddingToTaitank();
  return *this;
}

Container& Container::SetMargin(const EdgeInsets& margin) noexcept {
  margin_ = margin;
  ApplyMarginToTaitank();
  return *this;
}

Container& Container::SetWidth(float width) noexcept {
  fixed_width_ = width;
  auto* node = GetTaitankNode();
  if (node != nullptr) {
    taitank::SetWidth(node, width > 0.0F ? width : NAN);
  }
  return *this;
}

Container& Container::SetHeight(float height) noexcept {
  fixed_height_ = height;
  auto* node = GetTaitankNode();
  if (node != nullptr) {
    taitank::SetHeight(node, height > 0.0F ? height : NAN);
  }
  return *this;
}

Container& Container::SetChild(std::shared_ptr<Widget> child) {
  ClearChildren();
  AddChild(std::move(child));
  return *this;
}

Container& Container::SetFlexDirection(FlexDirection direction) noexcept {
  auto* node = GetTaitankNode();
  if (node != nullptr) {
    taitank::SetFlexDirection(node, ToTaitankDirection(direction));
  }
  return *this;
}

Container& Container::SetJustifyContent(HAlign align) noexcept {
  auto* node = GetTaitankNode();
  if (node != nullptr) {
    taitank::SetJustifyContent(node, ToTaitankAlign(align));
  }
  return *this;
}

Container& Container::SetAlignItems(VAlign align) noexcept {
  auto* node = GetTaitankNode();
  if (node != nullptr) {
    taitank::SetAlignItems(node, ToTaitankVAlign(align));
  }
  return *this;
}

Container& Container::SetFlexGrow(float grow) noexcept {
  auto* node = GetTaitankNode();
  if (node != nullptr) {
    taitank::SetFlexGrow(node, grow);
  }
  return *this;
}

Container& Container::SetBorderRadius(float radius) noexcept {
  border_radius_ = radius;
  return *this;
}

void Container::Paint(RenderContext& context) {
  if (has_background_) {
    if (border_radius_ > 0.0F) {
      context.DrawRoundedRect(
          {.x = 0.0F, .y = 0.0F, .width = bounds_.width,
           .height = bounds_.height},
          background_color_, border_radius_);
    } else {
      context.DrawRect(
          {.x = 0.0F, .y = 0.0F, .width = bounds_.width,
           .height = bounds_.height},
          background_color_);
    }
  }
  PaintChildren(context);
}

void Container::ApplyPaddingToTaitank() noexcept {
  auto* node = GetTaitankNode();
  if (node == nullptr) {
    return;
  }
  taitank::SetPadding(node, taitank::CSS_LEFT, padding_.left);
  taitank::SetPadding(node, taitank::CSS_TOP, padding_.top);
  taitank::SetPadding(node, taitank::CSS_RIGHT, padding_.right);
  taitank::SetPadding(node, taitank::CSS_BOTTOM, padding_.bottom);
}

void Container::ApplyMarginToTaitank() noexcept {
  auto* node = GetTaitankNode();
  if (node == nullptr) {
    return;
  }
  taitank::SetMargin(node, taitank::CSS_LEFT, margin_.left);
  taitank::SetMargin(node, taitank::CSS_TOP, margin_.top);
  taitank::SetMargin(node, taitank::CSS_RIGHT, margin_.right);
  taitank::SetMargin(node, taitank::CSS_BOTTOM, margin_.bottom);
}

}  // namespace neoflux
