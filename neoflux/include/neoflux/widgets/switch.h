// SPDX-License-Identifier: Apache-2.0
// This file is adapted from EUI-Neo (https://github.com/sudoevolve/EUI-NEO, commit 782c56993dc1890e0589e2100cfa74322bb0e0bf).
// NOTICE: see THIRD_PARTY_NOTICES.md for full upstream attribution.
// =============================================================================
// NeoFlux - switch.h
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
// A toggle switch: a pill track with a sliding knob that flips between two
// states. Clicking anywhere on the widget toggles it and notifies a callback.
//
// Pimpl: Switch::Impl (switch.cpp) holds the checked state, colors, geometry,
// and change callback. This header includes nothing but <memory> and the base.
// =============================================================================

#ifndef NEOFLUX_WIDGET_SWITCH_H_
#define NEOFLUX_WIDGET_SWITCH_H_

#include <memory>

#include "neoflux/widgets/widget.h"

namespace neoflux {

// A binary on/off toggle.
class Switch : public Widget {
 public:
  // Callback invoked with the new checked state whenever the switch toggles.
  using OnChanged = std::function<void(bool checked)>;

  Switch();
  explicit Switch(bool checked);
  ~Switch() override;

  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;

  // Sets the checked state programmatically (does not fire the callback).
  Switch& SetChecked(bool checked) noexcept;

  // Returns the current checked state.
  [[nodiscard]] bool IsChecked() const noexcept;

  // Sets the callback fired when the user toggles the switch.
  Switch& SetOnChanged(OnChanged callback) noexcept;

  // Sets the optional trailing label text.
  Switch& SetLabel(std::string label);

  // Sets the track color used when the switch is on.
  Switch& SetOnColor(const Color& color) noexcept;

  // Sets the track color used when the switch is off.
  Switch& SetOffColor(const Color& color) noexcept;

  // Reports intrinsic size to the Taitank layout engine.
  [[nodiscard]] Size OnMeasure(float width, int width_mode, float height,
                                int height_mode) override;

  void Paint(RenderContext& context) override;

  // Toggles the switch on pointer down.
  bool OnPointerDown(const Point& local_pos) override;

 private:
  // Sets the checked state and tweens the knob slide (~150ms). In headless
  // contexts (no event loop) the knob snaps to the target position.
  void ApplyChecked(bool checked);

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_SWITCH_H_
