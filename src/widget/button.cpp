// =============================================================================
// NeoFlux - button.cpp
//
// Implementation of Button widget. Methods moved from header.
// =============================================================================

#include "neoflux/widget/button.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>

#include <glog/logging.h>

#include "neoflux/core/types.h"
#include "neoflux/render/render_context.h"

namespace neoflux {

namespace {

float EstimateLabelWidth(std::string_view label, float font_size) {
  constexpr float kAvgCharWidthRatio = 0.55F;
  return (static_cast<float>(label.size()) * font_size) * kAvgCharWidthRatio;
}

}  // namespace

Button::Button(std::string label)
    : label_(std::move(label)),
      on_pressed_(),
      background_color_{.r = 0x21, .g = 0x96, .b = 0xF3, .a = 0xFF},
      text_color_{.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF},
      pressed_color_{.r = 0x19, .g = 0x76, .b = 0xD2, .a = 0xFF},
      font_size_(14.0F),
      horizontal_padding_(16.0F),
      vertical_padding_(8.0F),
      is_pressed_(false) {}

Button::~Button() = default;

std::string_view Button::GetWidgetName() const noexcept { return "Button"; }

Button& Button::SetLabel(std::string label) {
  label_ = std::move(label);
  MarkNeedsBuild();
  return *this;
}

std::string_view Button::GetLabel() const noexcept { return label_; }

Button& Button::SetOnPressed(OnPressed callback) noexcept {
  on_pressed_ = std::move(callback);
  return *this;
}

Button& Button::SetBackgroundColor(const Color& color) noexcept {
  background_color_ = color;
  return *this;
}

Button& Button::SetTextColor(const Color& color) noexcept {
  text_color_ = color;
  return *this;
}

Button& Button::SetFontSize(float size) noexcept {
  font_size_ = size;
  return *this;
}

bool Button::HandlePress(const Point& local_pos) {
  if (ContainsPoint(local_pos)) {
    is_pressed_ = true;
    MarkNeedsBuild();
    return true;
  }
  return false;
}

void Button::HandleRelease(const Point& local_pos) {
  if (is_pressed_ && ContainsPoint(local_pos)) {
    if (on_pressed_) {
      on_pressed_();
    }
  }
  is_pressed_ = false;
  MarkNeedsBuild();
}

Size Button::Layout(const LayoutConstraints& constraints) {
  const float label_width = EstimateLabelWidth(label_, font_size_);
  const float content_width = label_width + (2.0F * horizontal_padding_);
  const float content_height = (font_size_ * 1.2F) + (2.0F * vertical_padding_);

  float width =
      std::clamp(content_width, constraints.min_width, constraints.max_width);
  float height = std::clamp(content_height, constraints.min_height,
                            constraints.max_height);

  SetBounds({.x = bounds_.x, .y = bounds_.y, .width = width, .height = height});
  SetDesiredSize({.width = width, .height = height});
  return {.width = width, .height = height};
}

void Button::Paint(RenderContext& context) {
  const Color background = is_pressed_ ? pressed_color_ : background_color_;
  context.DrawRect({.x=0.0F, .y=0.0F, .width=bounds_.width, .height=bounds_.height}, background);

  const float label_width = EstimateLabelWidth(label_, font_size_);
  const float label_x = (bounds_.width - label_width) / 2.0F;  // NOLINT(cppcoreguidelines-init-variables)
  const float label_y = vertical_padding_ + font_size_;
  context.DrawText(label_, {.x=label_x, .y=label_y}, text_color_, font_size_);
}

bool Button::ContainsPoint(const Point& point) const noexcept {
  return point.x >= 0.0F && point.x <= bounds_.width && point.y >= 0.0F &&
         point.y <= bounds_.height;
}

}  // namespace neoflux
