// SPDX-License-Identifier: Apache-2.0
// This file is adapted from EUI-Neo (https://github.com/sudoevolve/EUI-NEO, commit 782c56993dc1890e0589e2100cfa74322bb0e0bf).
// NOTICE: see THIRD_PARTY_NOTICES.md for full upstream attribution.
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

#include "neoflux/widgets/switch.h"

#include <algorithm>
#include <chrono>
#include <string>
#include <string_view>
#include <utility>

#include "neoflux/core/animation.h"
#include "neoflux/core/types.h"
#include "neoflux/renderers/render_context.h"

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

struct Switch::Impl {
  OnChanged on_changed;
  std::string label;
  bool checked = false;
  Color on_color{.r = 0x21, .g = 0x96, .b = 0xF3, .a = 0xFF};
  Color off_color{.r = 0xBD, .g = 0xBD, .b = 0xBD, .a = 0xFF};
  Color knob_color{.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF};
  float track_width = 52.0F;
  float track_height = 30.0F;
  float font_size = 14.0F;
  // Animated knob position: 0 = off (left), 1 = on (right). Driven by a
  // Sleep()-ticked coroutine; snaps to the target in headless contexts.
  float knob_progress = 0.0F;
  AnimationRuntime anim;
};

// Knob slide duration for the toggle animation.
constexpr std::chrono::milliseconds kToggleDuration{150};

Switch::Switch() : impl_(std::make_unique<Impl>()) {
  EnableMeasureFunction();
}

Switch::Switch(bool checked) : impl_(std::make_unique<Impl>()) {
  impl_->checked = checked;
  impl_->knob_progress = checked ? 1.0F : 0.0F;
  EnableMeasureFunction();
}

Switch::~Switch() = default;

std::string_view Switch::GetWidgetName() const noexcept { return "Switch"; }

Switch& Switch::SetChecked(bool checked) noexcept {
  ApplyChecked(checked);
  return *this;
}

bool Switch::IsChecked() const noexcept { return impl_->checked; }

void Switch::ApplyChecked(bool checked) {
  impl_->checked = checked;
  impl_->anim.Bind(weak_from_this());
  const float target = checked ? 1.0F : 0.0F;
  if (impl_->anim.CanAnimate()) {
    impl_->anim.Tween(impl_->knob_progress, target, kToggleDuration,
                      [this](float progress) {
                        impl_->knob_progress = progress;
                        MarkNeedsBuild();
                      });
  } else {
    // Headless (no event loop / stack-allocated): snap to the final position.
    impl_->knob_progress = target;
  }
  MarkNeedsBuild();
}

Switch& Switch::SetOnChanged(OnChanged callback) noexcept {
  impl_->on_changed = std::move(callback);
  return *this;
}

Switch& Switch::SetLabel(std::string label) {
  impl_->label = std::move(label);
  MarkNeedsBuild();
  return *this;
}

Switch& Switch::SetOnColor(const Color& color) noexcept {
  impl_->on_color = color;
  return *this;
}

Switch& Switch::SetOffColor(const Color& color) noexcept {
  impl_->off_color = color;
  return *this;
}

Size Switch::OnMeasure(float width, int width_mode, float height,
                      int height_mode) {
  const float label_width = impl_->label.empty()
                                ? 0.0F
                                : kLabelGap +
                                      EstimateLabelWidth(impl_->label,
                                                         impl_->font_size);
  const float intrinsic_width = impl_->track_width + label_width;
  const float intrinsic_height =
      std::max(impl_->track_height, impl_->font_size * 1.3F);

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
  const float track_y = (bounds_.height - impl_->track_height) * 0.5F;
  const Color track_color = impl_->checked ? impl_->on_color : impl_->off_color;
  context.DrawRoundedRect(
      {.x = 0.0F, .y = track_y, .width = impl_->track_width,
       .height = impl_->track_height},
      track_color, impl_->track_height * 0.5F);

  // Knob geometry: inset by a small margin inside the track, travelling from
  // the left edge (off) to the right edge (on).
  const float margin = impl_->track_height * 0.125F;
  const float knob_size = impl_->track_height - (2.0F * margin);
  const float knob_travel =
      impl_->track_width - (2.0F * margin) - knob_size;
  const float knob_x = margin + (impl_->knob_progress * knob_travel);
  context.DrawRoundedRect(
      {.x = knob_x, .y = track_y + margin, .width = knob_size,
       .height = knob_size},
      impl_->knob_color, knob_size * 0.5F);

  if (!impl_->label.empty()) {
    const float baseline_y =
        track_y + ((impl_->track_height + impl_->font_size) * 0.5F);
    context.DrawText(impl_->label,
                     {.x = impl_->track_width + kLabelGap, .y = baseline_y},
                     Color{.r = 0x33, .g = 0x33, .b = 0x33, .a = 0xFF},
                     impl_->font_size, "");
  }
}

bool Switch::OnPointerDown(const Point& /*local_pos*/) {
  ApplyChecked(!impl_->checked);
  if (impl_->on_changed) {
    impl_->on_changed(impl_->checked);
  }
  return true;
}

}  // namespace neoflux
