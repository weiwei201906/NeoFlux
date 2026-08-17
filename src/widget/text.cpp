// =============================================================================
// NeoFlux - text.cpp
//
// Implementation of Text widget. Intrinsic size is reported to Taitank via
// OnMeasure(); rendering is delegated to the RenderContext (tgfx).
// =============================================================================

#include "neoflux/widget/text.h"

#include <glog/logging.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include "neoflux/core/types.h"
#include "neoflux/render/render_context.h"

namespace neoflux {

namespace {

// Estimates the rendered width of a UTF-8 string. CJK code points are
// approximately font_size wide; Latin characters are ~0.55 * font_size.
// This is a heuristic for layout; tgfx performs exact glyph measurement.
float EstimateTextWidth(std::string_view text, float font_size) {
  constexpr float kLatinWidthRatio = 0.55F;
  float width = 0.0F;
  for (std::size_t i = 0; i < text.size();) {
    const auto byte = static_cast<unsigned char>(text[i]);
    std::size_t char_len = 1;
    if ((byte & 0x80U) == 0U) {
      char_len = 1;  // ASCII
    } else if ((byte & 0xE0U) == 0xC0U) {
      char_len = 2;
    } else if ((byte & 0xF0U) == 0xE0U) {
      char_len = 3;  // CJK BMP
    } else if ((byte & 0xF8U) == 0xF0U) {
      char_len = 4;
    }
    width += (char_len > 1) ? font_size : font_size * kLatinWidthRatio;
    i += char_len;
  }
  return width;
}

}  // namespace

Text::Text(std::string text)
    : text_(std::move(text)),
      text_color_{.r = 0, .g = 0, .b = 0, .a = 255},
      font_size_(14.0F),
      alignment_(HAlign::kLeft) {
  // Text is a leaf node: enable the Taitank measure function so the layout
  // engine queries this widget's intrinsic size.
  EnableMeasureFunction();
}

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

Text& Text::SetFont(std::string_view font_name) {
  font_name_ = std::string(font_name);
  return *this;
}

Size Text::OnMeasure(float width, int width_mode, float height,
                     int height_mode) {
  const float intrinsic_width = EstimateTextWidth(text_, font_size_);
  const float intrinsic_height = font_size_ * 1.3F;

  // Taitank measure modes: 0=undefined, 1=exactly, 2=at_most.
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

void Text::Paint(RenderContext& context) {
  const float text_width = EstimateTextWidth(text_, font_size_);
  const float text_x =
      (alignment_ == HAlign::kCenter)
          ? (bounds_.width - text_width) / 2.0F
          : (alignment_ == HAlign::kRight ? bounds_.width - text_width : 0.0F);
  // Baseline at font_size from top (approximate descent).
  context.DrawText(text_, {.x = text_x, .y = font_size_}, text_color_,
                   font_size_, font_name_);
}

}  // namespace neoflux
