// =============================================================================
// NeoFlux - switch.cpp
//
// NOTICE: This widget was designed with reference to the Switch component of
// EUI-NEO (https://github.com/sudoevolve/EUI-NEO), Copyright sudoevolve,
// licensed under the Apache License, Version 2.0. The component's behavior
// (click flips the checked state and fires a change callback) and geometry
// (a rounded pill track with a travelling circular knob, plus an optional
// trailing label) were adapted to the NeoFlux widget/render model. No
// EUI-NEO source code was copied.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy
// of the License at http://www.apache.org/licenses/LICENSE-2.0
//
// Implementation of Switch. Intrinsic size is reported to Taitank via
// OnMeasure(); the track and knob are drawn as rounded rects.
// =============================================================================

#include "neoflux/widget/switch.h"

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
// wide; Latin characters are ~0.55 * font_size. Mirrors the heuristic used by
// the Button and Text widgets. Iterates per byte: continuation bytes are
// skipped and only leading bytes contribute width.
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

// Gap between the track and the trailing label.
constexpr float kLabelGap = 8.0F;

}  // namespace

Switch::Switch() { EnableMeasureFunction(); }

Switch::Switch(bool checked) : checked_(checked) { EnableMeasureFunction(); }

Switch::~Switch() = default;

std::string_view Switch::GetWidgetName() const noexcept { return "Switch"; }

Switch& Switch::SetChecked(bool checked) noexcept {
  checked_ = checked;
  MarkNeedsBuild();
  return *this;
}

bool Switch::IsChecked() const noexcept { return checked_; }

Switch& Switch::SetOnChanged(OnChanged callback) noexcept {
  on_changed_ = std::move(callback);
  return *this;
}

Switch& Switch::SetLabel(std::string label) {
  label_ = std::move(label);
  MarkNeedsBuild();
  return *this;
}

Switch& Switch::SetOnColor(const Color& color) noexcept {
  on_color_ = color;
  return *this;
}

Switch& Switch::SetOffColor(const Color& color) noexcept {
  off_color_ = color;
  return *this;
}

Size Switch::OnMeasure(float width, int width_mode, float height,
                      int height_mode) {
  const float label_width =
      label_.empty() ? 0.0F : kLabelGap + EstimateLabelWidth(label_, font_size_);
  const float intrinsic_width = track_width_ + label_width;
  const float intrinsic_height =
      std::max(track_height_, font_size_ * 1.3F);

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

void Switch::Paint(RenderContext& context) {
  const float track_y = (bounds_.height - track_height_) * 0.5F;
  const Color track_color = checked_ ? on_color_ : off_color_;
  context.DrawRoundedRect(
      {.x = 0.0F, .y = track_y, .width = track_width_, .height = track_height_},
      track_color, track_height_ * 0.5F);

  // Knob geometry: inset by a small margin inside the track, travelling from
  // the left edge (off) to the right edge (on).
  const float margin = track_height_ * 0.125F;
  const float knob_size = track_height_ - (2.0F * margin);
  const float knob_travel = track_width_ - (2.0F * margin) - knob_size;
  const float knob_x = margin + (checked_ ? knob_travel : 0.0F);
  context.DrawRoundedRect(
      {.x = knob_x, .y = track_y + margin, .width = knob_size,
       .height = knob_size},
      knob_color_, knob_size * 0.5F);

  if (!label_.empty()) {
    const float baseline_y =
        track_y + ((track_height_ + font_size_) * 0.5F);
    context.DrawText(label_,
                     {.x = track_width_ + kLabelGap, .y = baseline_y},
                     Color{.r = 0x33, .g = 0x33, .b = 0x33, .a = 0xFF},
                     font_size_, "");
  }
}

bool Switch::OnPointerDown(const Point& /*local_pos*/) {
  Toggle();
  return true;
}

void Switch::Toggle() {
  checked_ = !checked_;
  MarkNeedsBuild();
  if (on_changed_) {
    on_changed_(checked_);
  }
}

}  // namespace neoflux
