// =============================================================================
// NeoFlux - text.cpp
//
// Implementation of Text widget. Methods moved from header.
// =============================================================================

#include "neoflux/widget/text.h"

#include <algorithm>
#include <string>
#include <utility>

#include "neoflux/render/render_context.h"

namespace neoflux {

namespace {

float EstimateTextWidth(std::string_view text, float font_size) {
  constexpr float kAvgCharWidthRatio = 0.55F;
  return static_cast<float>(text.size()) * font_size * kAvgCharWidthRatio;
}

}  // namespace

Text::Text(std::string text)
    : text_(std::move(text)),
      text_color_{.r = 0, .g = 0, .b = 0, .a = 255},
      font_size_(14.0F),
      alignment_(HAlign::kLeft) {}

Text::~Text() = default;

std::string_view Text::GetWidgetName() const noexcept { return "Text"; }

Text& Text::SetText(std::string text) {
  text_ = std::move(text);
  MarkNeedsBuild();
  return *this;
}

std::string_view Text::GetText() const noexcept { return text_; }

Text& Text::SetTextColor(const Color& color) noexcept {
  text_color_ = color;
  return *this;
}

Text& Text::SetFontSize(float size) noexcept {
  font_size_ = size;
  return *this;
}

Text& Text::SetAlignment(HAlign align) noexcept {
  alignment_ = align;
  return *this;
}

Size Text::Layout(const LayoutConstraints& constraints) {
  const float text_width = EstimateTextWidth(text_, font_size_);
  const float text_height = font_size_ * 1.2F;

  float width =
      std::clamp(text_width, constraints.min_width, constraints.max_width);
  float height = std::clamp(text_height, constraints.min_height,
                            constraints.max_height);

  SetBounds({.x = bounds_.x, .y = bounds_.y, .width = width, .height = height});
  SetDesiredSize({.width = width, .height = height});
  return {.width = width, .height = height};
}

void Text::Paint(RenderContext& context) {
  float x = 0.0F;
  if (alignment_ == HAlign::kCenter) {
    x = (bounds_.width - EstimateTextWidth(text_, font_size_)) / 2.0F;
  } else if (alignment_ == HAlign::kRight) {
    x = bounds_.width - EstimateTextWidth(text_, font_size_);
  }

  context.DrawText(text_, {.x=x, .y=font_size_}, text_color_, font_size_);
}

}  // namespace neoflux
