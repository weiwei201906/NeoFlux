// =============================================================================
// NeoFlux - container.cpp
//
// Implementation of Container widget. Methods moved from header.
// =============================================================================

#include "neoflux/widget/container.h"

#include <algorithm>

#include "neoflux/render/render_context.h"

namespace neoflux {

Container::Container()
    : background_color_(),
      padding_(),
      margin_(),
      fixed_width_(0.0F),
      fixed_height_(0.0F),
      alignment_(HAlign::kLeft),
      has_background_(false) {}

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
  return *this;
}

Container& Container::SetMargin(const EdgeInsets& margin) noexcept {
  margin_ = margin;
  return *this;
}

Container& Container::SetWidth(float width) noexcept {
  fixed_width_ = width;
  return *this;
}

Container& Container::SetHeight(float height) noexcept {
  fixed_height_ = height;
  return *this;
}

Container& Container::SetChild(std::shared_ptr<Widget> child) {
  ClearChildren();
  AddChild(std::move(child));
  return *this;
}

Container& Container::SetAlignment(HAlign align) noexcept {
  alignment_ = align;
  return *this;
}

Size Container::Layout(const LayoutConstraints& constraints) {
  LayoutConstraints inner = constraints;
  inner.min_width =
      std::max(0.0F, constraints.min_width - padding_.left - padding_.right);
  inner.max_width =
      std::max(0.0F, constraints.max_width - padding_.left - padding_.right);
  inner.min_height =
      std::max(0.0F, constraints.min_height - padding_.top - padding_.bottom);
  inner.max_height =
      std::max(0.0F, constraints.max_height - padding_.top - padding_.bottom);

  float content_width = 0.0F;
  float content_height = 0.0F;

  if (!GetChildren().empty() && GetChildren().front() != nullptr) {
    auto& child = GetChildren().front();
    const Size child_size = child->Layout(inner);
    content_width = child_size.width;
    content_height = child_size.height;

    float child_x = padding_.left;
    if (alignment_ == HAlign::kCenter) {
      child_x = padding_.left +
                std::max(0.0F, (inner.max_width - content_width) / 2.0F);
    } else if (alignment_ == HAlign::kRight) {
      child_x = inner.max_width - content_width - padding_.right;
    }
    child->SetBounds({child_x, padding_.top, content_width, content_height});
  }

  float width = fixed_width_ > 0.0F
                    ? fixed_width_
                    : content_width + padding_.left + padding_.right;
  float height = fixed_height_ > 0.0F
                     ? fixed_height_
                     : content_height + padding_.top + padding_.bottom;

  width = std::clamp(width, constraints.min_width, constraints.max_width);
  height = std::clamp(height, constraints.min_height, constraints.max_height);

  SetBounds({bounds_.x, bounds_.y, width, height});
  SetDesiredSize({width, height});
  return {width, height};
}

void Container::Paint(RenderContext& context) {
  if (has_background_) {
    context.DrawRect({0.0F, 0.0F, bounds_.width, bounds_.height},
                     background_color_);
  }
  PaintChildren(context);
}

}  // namespace neoflux
