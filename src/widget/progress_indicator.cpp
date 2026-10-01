// =============================================================================
// NeoFlux - progress_indicator.cpp
//
// NOTICE: This widget was designed with reference to the Progress component of
// EUI-NEO (https://github.com/sudoevolve/EUI-NEO), Copyright sudoevolve,
// licensed under the Apache License, Version 2.0. The component's structure
// (a rounded track bar with a filled portion whose width is proportional to a
// clamped 0..1 value) was adapted to the NeoFlux widget/render model. No
// EUI-NEO source code was copied.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy
// of the License at http://www.apache.org/licenses/LICENSE-2.0
//
// Implementation of ProgressIndicator. Intrinsic size is reported to Taitank
// via OnMeasure(); the track and fill are drawn as rounded rects.
// =============================================================================

#include "neoflux/widget/progress_indicator.h"

#include <algorithm>
#include <cmath>

#include "neoflux/core/types.h"
#include "neoflux/render/render_context.h"

namespace neoflux {

namespace {
// Natural width when the parent does not constrain the bar.
constexpr float kDefaultWidth = 200.0F;
}  // namespace

ProgressIndicator::ProgressIndicator() {
  // Leaf node: report intrinsic size to the layout engine.
  EnableMeasureFunction();
}

ProgressIndicator::~ProgressIndicator() = default;

std::string_view ProgressIndicator::GetWidgetName() const noexcept {
  return "ProgressIndicator";
}

ProgressIndicator& ProgressIndicator::SetValue(float value) noexcept {
  value_ = std::clamp(value, 0.0F, 1.0F);
  MarkNeedsBuild();
  return *this;
}

float ProgressIndicator::GetValue() const noexcept { return value_; }

ProgressIndicator& ProgressIndicator::SetTrackColor(const Color& color) noexcept {
  track_color_ = color;
  return *this;
}

ProgressIndicator& ProgressIndicator::SetFillColor(const Color& color) noexcept {
  fill_color_ = color;
  return *this;
}

ProgressIndicator& ProgressIndicator::SetThickness(float thickness) noexcept {
  thickness_ = thickness > 0.0F ? thickness : 1.0F;
  return *this;
}

Size ProgressIndicator::OnMeasure(float width, int width_mode, float height,
                                  int height_mode) {
  float measured_width = kDefaultWidth;
  float measured_height = thickness_;
  if (width_mode == 1) {  // exactly
    measured_width = width;
  } else if (width_mode == 2) {  // at_most
    measured_width = std::min(kDefaultWidth, width);
  }
  if (height_mode == 1) {
    measured_height = height;
  } else if (height_mode == 2) {
    measured_height = std::min(thickness_, height);
  }
  return {.width = measured_width, .height = measured_height};
}

void ProgressIndicator::Paint(RenderContext& context) {
  const float height = bounds_.height;
  const float track_radius = height * 0.5F;
  context.DrawRoundedRect(
      {.x = 0.0F, .y = 0.0F, .width = bounds_.width, .height = height},
      track_color_, track_radius);

  const float fill_width = bounds_.width * value_;
  if (fill_width <= 0.0F) {
    return;
  }
  // Clamp the corner radius to half the fill width so a short fill segment is
  // not over-rounded.
  const float fill_radius = std::min(track_radius, fill_width * 0.5F);
  context.DrawRoundedRect({.x = 0.0F, .y = 0.0F, .width = fill_width,
                           .height = height},
                          fill_color_, fill_radius);
}

}  // namespace neoflux
