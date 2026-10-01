// =============================================================================
// NeoFlux - checkbox.h
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
// A binary checked/unchecked box with an optional trailing label.
//
// Pimpl: Checkbox::Impl (checkbox.cpp) holds the checked state, colors, box
// metrics, label, and change callback. This header includes nothing but
// <memory> and the widget base.
// =============================================================================

#ifndef NEOFLUX_WIDGET_CHECKBOX_H_
#define NEOFLUX_WIDGET_CHECKBOX_H_

#include <memory>

#include "neoflux/widgets/widget.h"

namespace neoflux {

// A square checkbox that flips between checked and unchecked.
class Checkbox : public Widget {
 public:
  // Callback invoked with the new checked state whenever the box toggles.
  using OnChanged = std::function<void(bool checked)>;

  Checkbox();
  explicit Checkbox(bool checked);
  ~Checkbox() override;

  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;

  // Sets the checked state programmatically (does not fire the callback).
  Checkbox& SetChecked(bool checked) noexcept;

  // Returns the current checked state.
  [[nodiscard]] bool IsChecked() const noexcept;

  // Sets the callback fired when the user toggles the box.
  Checkbox& SetOnChanged(OnChanged callback) noexcept;

  // Sets the optional trailing label text.
  Checkbox& SetLabel(std::string label);

  // Sets the box color used when checked.
  Checkbox& SetCheckedColor(const Color& color) noexcept;

  // Reports intrinsic size to the Taitank layout engine.
  [[nodiscard]] Size OnMeasure(float width, int width_mode, float height,
                                int height_mode) override;

  void Paint(RenderContext& context) override;

  // Toggles the box on pointer down.
  bool OnPointerDown(const Point& local_pos) override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_CHECKBOX_H_
