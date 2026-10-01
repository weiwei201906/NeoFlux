// =============================================================================
// NeoFlux - progress_indicator.h
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
// A horizontal progress bar: a rounded track with a filled portion drawn from
// the left, proportional to the current value in [0, 1].
//
// Pimpl: ProgressIndicator::Impl (progress_indicator.cpp) holds the value and
// colors. This header includes nothing but <memory> and the widget base.
// =============================================================================

#ifndef NEOFLUX_WIDGET_PROGRESS_INDICATOR_H_
#define NEOFLUX_WIDGET_PROGRESS_INDICATOR_H_

#include <memory>

#include "neoflux/widget/widget.h"

namespace neoflux {

// A determinate horizontal progress bar.
class ProgressIndicator : public Widget {
 public:
  ProgressIndicator();
  ~ProgressIndicator() override;

  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;

  // Sets the progress value in [0, 1]; values outside the range are clamped.
  ProgressIndicator& SetValue(float value) noexcept;

  // Returns the current clamped progress value.
  [[nodiscard]] float GetValue() const noexcept;

  // Sets the track (background) color.
  ProgressIndicator& SetTrackColor(const Color& color) noexcept;

  // Sets the filled portion color.
  ProgressIndicator& SetFillColor(const Color& color) noexcept;

  // Sets the bar thickness (track height) in pixels.
  ProgressIndicator& SetThickness(float thickness) noexcept;

  // Reports intrinsic bar size to the Taitank layout engine.
  [[nodiscard]] Size OnMeasure(float width, int width_mode, float height,
                               int height_mode) override;

  void Paint(RenderContext& context) override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_PROGRESS_INDICATOR_H_
