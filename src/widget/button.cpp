// =============================================================================
// NeoFlux - button.cpp
//
// Implementation of Button widget. Intrinsic size (label + padding) is
// reported to Taitank via OnMeasure(); rendering uses tgfx through
// RenderContext.
// =============================================================================

#include "neoflux/widget/button.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include <glog/logging.h>

#include "neoflux/core/types.h"
#include "neoflux/render/render_context.h"

namespace neoflux {

namespace {

// Estimates the rendered width of a UTF-8 label. CJK code points are
// approximately font_size wide; Latin characters are ~0.55 * font_size.
float EstimateLabelWidth(std::string_view label, float font_size) {
  constexpr float kLatinWidthRatio = 0.55F;
  float width = 0.0F;
  for (std::size_t i = 0; i < label.size();) {
    const auto byte = static_cast<unsigned char>(label[i]);
    std::size_t char_len = 1;
    if ((byte & 0x80U) == 0U) {
      char_len = 1;
    } else if ((byte & 0xE0U) == 0xC0U) {
      char_len = 2;
    } else if ((byte & 0xF0U) == 0xE0U) {
      char_len = 3;
    } else if ((byte & 0xF8U) == 0xF0U) {
      char_len = 4;
    }
    width += (char_len > 1) ? font_size : font_size * kLatinWidthRatio;
    i += char_len;
  }
  return width;
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
      is_pressed_(false) {
  // Button is a leaf node: enable the Taitank measure function.
  EnableMeasureFunction();
}

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
  if (is_pressed_ && ContainsPoint(local_pos) && on_pressed_) {
    on_pressed_();
  }
  is_pressed_ = false;
  MarkNeedsBuild();
}

bool Button::OnPointerDown(const Point& local_pos) {
  return HandlePress(local_pos);
}

void Button::OnPointerUp(const Point& local_pos) {
  HandleRelease(local_pos);
}

Size Button::OnMeasure(float width, int width_mode, float height,
                       int height_mode) {
  const float label_width = EstimateLabelWidth(label_, font_size_);
  const float intrinsic_width = label_width + (2.0F * horizontal_padding_);
  const float intrinsic_height =
      (font_size_ * 1.3F) + (2.0F * vertical_padding_);

  float measured_width = intrinsic_width;
  float measured_height = intrinsic_height;
  if (width_mode == 1) {  // exactly
    measured_width = width;
  } else if (width_mode == 2) {  // at_most
    measured_width = std::min(intrinsic_width, width);
  }
  if (height_mode == 1) {
    measured_height = height;
  } else if (height_mode == 2) {
    measured_height = std::min(intrinsic_height, height);
  }
  return {.width = measured_width, .height = measured_height};
}

void Button::Paint(RenderContext& context) {
  const Color background = is_pressed_ ? pressed_color_ : background_color_;
  context.DrawRect(
      {.x = 0.0F, .y = 0.0F, .width = bounds_.width, .height = bounds_.height},
      background);

  const float label_width = EstimateLabelWidth(label_, font_size_);
  const float label_x = (bounds_.width - label_width) / 2.0F;
  const float label_y = vertical_padding_ + font_size_;
  context.DrawText(label_, {.x = label_x, .y = label_y}, text_color_,
                   font_size_);
}

bool Button::ContainsPoint(const Point& point) const noexcept {
  return point.x >= 0.0F && point.x <= bounds_.width && point.y >= 0.0F &&
         point.y <= bounds_.height;
}

}  // namespace neoflux
