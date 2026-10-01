// =============================================================================
// NeoFlux - checkbox.cpp
//
// NOTICE: This widget was designed with reference to the Checkbox component of
// EUI-NEO (https://github.com/sudoevolve/EUI-NEO), Copyright sudoevolve,
// licensed under the Apache License, Version 2.0. The component's behavior
// (click flips the checked state and fires a change callback) and structure
// (a rounded square indicator box with a checked mark and a trailing label)
// were adapted to the NeoFlux widget/render model. Because NeoFlux's render
// command buffer exposes only rectangles (no polygon/line primitives), the
// check mark is rendered as a centered filled block rather than the source's
// polygon tick. No EUI-NEO source code was copied.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy
// of the License at http://www.apache.org/licenses/LICENSE-2.0
//
// Implementation of Checkbox. Intrinsic size is reported to Taitank via
// OnMeasure(); the box and check mark are drawn as rounded rects.
// =============================================================================

#include "neoflux/widget/checkbox.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include "neoflux/core/types.h"
#include "neoflux/render/render_context.h"

namespace neoflux {

namespace {

// Estimates the rendered width of a UTF-8 label. CJK code points are ~font_size
// wide; Latin characters are ~0.55 * font_size. Mirrors Button/Text. Iterates
// per byte: continuation bytes are skipped and only leading bytes count.
float EstimateLabelWidth(std::string_view label, float font_size) {
  constexpr float kLatinWidthRatio = 0.55F;
  float width = 0.0F;
  for (const char ch : label) {
    const auto byte = static_cast<unsigned char>(ch);
    // UTF-8 continuation bytes (10xxxxxx) belong to the previous leading byte.
    if ((byte & 0xC0U) == 0x80U) {
      continue;
    }
    width += ((byte & 0x80U) == 0U) ? font_size * kLatinWidthRatio : font_size;
  }
  return width;
}

// Gap between the box and the trailing label.
constexpr float kLabelGap = 8.0F;
// Corner radius of the indicator box.
constexpr float kBoxRadius = 4.0F;

}  // namespace

Checkbox::Checkbox() { EnableMeasureFunction(); }

Checkbox::Checkbox(bool checked) : checked_(checked) { EnableMeasureFunction(); }

Checkbox::~Checkbox() = default;

std::string_view Checkbox::GetWidgetName() const noexcept { return "Checkbox"; }

Checkbox& Checkbox::SetChecked(bool checked) noexcept {
  checked_ = checked;
  MarkNeedsBuild();
  return *this;
}

bool Checkbox::IsChecked() const noexcept { return checked_; }

Checkbox& Checkbox::SetOnChanged(OnChanged callback) noexcept {
  on_changed_ = std::move(callback);
  return *this;
}

Checkbox& Checkbox::SetLabel(std::string label) {
  label_ = std::move(label);
  MarkNeedsBuild();
  return *this;
}

Checkbox& Checkbox::SetCheckedColor(const Color& color) noexcept {
  checked_color_ = color;
  return *this;
}

Size Checkbox::OnMeasure(float width, int width_mode, float height,
                        int height_mode) {
  const float label_width =
      label_.empty() ? 0.0F : kLabelGap + EstimateLabelWidth(label_, font_size_);
  const float intrinsic_width = box_size_ + label_width;
  const float intrinsic_height =
      std::max(box_size_, font_size_ * 1.3F);

  float measured_width = intrinsic_width;
  float measured_height = intrinsic_height;
  if (width_mode == 1) {
    measured_width = width;
  } else if (width_mode == 2) {
    measured_width = std::min(intrinsic_width, width);
  }
  if (height_mode == 1) {
    measured_height = height;
  } else if (height_mode == 2) {
    measured_height = std::min(intrinsic_height, height);
  }
  return {.width = measured_width, .height = measured_height};
}

void Checkbox::Paint(RenderContext& context) {
  const float box_y = (bounds_.height - box_size_) * 0.5F;
  const Color box_fill = checked_ ? checked_color_ : box_color_;
  context.DrawRoundedRect(
      {.x = 0.0F, .y = box_y, .width = box_size_, .height = box_size_},
      box_fill, kBoxRadius);

  // With only rectangle primitives available, render the checked mark as a
  // centered filled block inside the box.
  if (checked_) {
    const float mark_size = box_size_ * 0.5F;
    const float mark_x = (box_size_ - mark_size) * 0.5F;
    const float mark_y = box_y + ((box_size_ - mark_size) * 0.5F);
    context.DrawRoundedRect(
        {.x = mark_x, .y = mark_y, .width = mark_size, .height = mark_size},
        mark_color_, mark_size * 0.25F);
  }

  if (!label_.empty()) {
    const float baseline_y =
        box_y + ((box_size_ + font_size_) * 0.5F);
    context.DrawText(label_,
                     {.x = box_size_ + kLabelGap, .y = baseline_y},
                     Color{.r = 0x33, .g = 0x33, .b = 0x33, .a = 0xFF},
                     font_size_, "");
  }
}

bool Checkbox::OnPointerDown(const Point& /*local_pos*/) {
  Toggle();
  return true;
}

void Checkbox::Toggle() {
  checked_ = !checked_;
  MarkNeedsBuild();
  if (on_changed_) {
    on_changed_(checked_);
  }
}

}  // namespace neoflux
