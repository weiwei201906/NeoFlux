// SPDX-License-Identifier: Apache-2.0
// This file is adapted from EUI-Neo (https://github.com/sudoevolve/EUI-NEO, commit 782c56993dc1890e0589e2100cfa74322bb0e0bf).
// NOTICE: see THIRD_PARTY_NOTICES.md for full upstream attribution.
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

#include "neoflux/widgets/checkbox.h"

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

struct Checkbox::Impl {
  OnChanged on_changed;
  std::string label;
  bool checked = false;
  Color checked_color{.r = 0x21, .g = 0x96, .b = 0xF3, .a = 0xFF};
  Color box_color{.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF};
  Color mark_color{.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF};
  float box_size = 22.0F;
  float font_size = 14.0F;
  // Animated check-mark reveal: 0 = hidden, 1 = fully shown. Driven by a
  // Sleep()-ticked coroutine; snaps to the target in headless contexts.
  float check_progress = 0.0F;
  AnimationRuntime anim;
};

// Check-mark reveal duration for the toggle animation.
constexpr std::chrono::milliseconds kCheckDuration{150};

Checkbox::Checkbox() : impl_(std::make_unique<Impl>()) {
  EnableMeasureFunction();
}

Checkbox::Checkbox(bool checked) : impl_(std::make_unique<Impl>()) {
  impl_->checked = checked;
  impl_->check_progress = checked ? 1.0F : 0.0F;
  EnableMeasureFunction();
}

Checkbox::~Checkbox() = default;

std::string_view Checkbox::GetWidgetName() const noexcept { return "Checkbox"; }

Checkbox& Checkbox::SetChecked(bool checked) noexcept {
  ApplyChecked(checked);
  return *this;
}

bool Checkbox::IsChecked() const noexcept { return impl_->checked; }

void Checkbox::ApplyChecked(bool checked) {
  impl_->checked = checked;
  impl_->anim.Bind(weak_from_this());
  const float target = checked ? 1.0F : 0.0F;
  if (impl_->anim.CanAnimate()) {
    impl_->anim.Tween(impl_->check_progress, target, kCheckDuration,
                      [this](float progress) {
                        impl_->check_progress = progress;
                        MarkNeedsBuild();
                      });
  } else {
    impl_->check_progress = target;
  }
  MarkNeedsBuild();
}

Checkbox& Checkbox::SetOnChanged(OnChanged callback) noexcept {
  impl_->on_changed = std::move(callback);
  return *this;
}

Checkbox& Checkbox::SetLabel(std::string label) {
  impl_->label = std::move(label);
  MarkNeedsBuild();
  return *this;
}

Checkbox& Checkbox::SetCheckedColor(const Color& color) noexcept {
  impl_->checked_color = color;
  return *this;
}

Size Checkbox::OnMeasure(float width, int width_mode, float height,
                        int height_mode) {
  const float label_width =
      impl_->label.empty()
          ? 0.0F
          : kLabelGap + EstimateLabelWidth(impl_->label, impl_->font_size);
  const float intrinsic_width = impl_->box_size + label_width;
  const float intrinsic_height =
      std::max(impl_->box_size, impl_->font_size * 1.3F);

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
  const float box_y = (bounds_.height - impl_->box_size) * 0.5F;
  const Color box_fill = impl_->checked ? impl_->checked_color : impl_->box_color;
  context.DrawRoundedRect(
      {.x = 0.0F, .y = box_y, .width = impl_->box_size,
       .height = impl_->box_size},
      box_fill, kBoxRadius);

  // With only rectangle primitives available, render the checked mark as a
  // centered filled block inside the box. The block scales in with the
  // animated check-progress (0 = hidden, 1 = fully shown).
  if (impl_->check_progress > 0.0F) {
    const float mark_size = impl_->box_size * 0.5F * impl_->check_progress;
    const float mark_x = (impl_->box_size - mark_size) * 0.5F;
    const float mark_y = box_y + ((impl_->box_size - mark_size) * 0.5F);
    context.DrawRoundedRect(
        {.x = mark_x, .y = mark_y, .width = mark_size, .height = mark_size},
        impl_->mark_color, mark_size * 0.25F);
  }

  if (!impl_->label.empty()) {
    const float baseline_y =
        box_y + ((impl_->box_size + impl_->font_size) * 0.5F);
    context.DrawText(impl_->label,
                     {.x = impl_->box_size + kLabelGap, .y = baseline_y},
                     Color{.r = 0x33, .g = 0x33, .b = 0x33, .a = 0xFF},
                     impl_->font_size, "");
  }
}

bool Checkbox::OnPointerDown(const Point& /*local_pos*/) {
  ApplyChecked(!impl_->checked);
  if (impl_->on_changed) {
    impl_->on_changed(impl_->checked);
  }
  return true;
}

}  // namespace neoflux
