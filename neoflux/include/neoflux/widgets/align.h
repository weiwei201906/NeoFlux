// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - align.h
//
// A single-child widget that expands to fill its parent and positions its child
// at a chosen horizontal/vertical alignment (e.g. top-left, bottom-right).
// More general than Center, which is equivalent to Align(kCenter, kCenter).
//
// Pimpl: Align::Impl (align.cpp) holds the alignment state. This header
// includes nothing but <memory> and the widget base.
// =============================================================================

#ifndef NEOFLUX_WIDGET_ALIGN_H_
#define NEOFLUX_WIDGET_ALIGN_H_

#include <memory>

#include "neoflux/core/types.h"
#include "neoflux/widgets/container.h"

namespace neoflux {

// Positions its child within the space it is given.
class Align : public Container {
 public:
  Align(HAlign horizontal, VAlign vertical,
        std::shared_ptr<Widget> child = nullptr);
  ~Align() override;

  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;

  // Changes the horizontal alignment.
  Align& SetHorizontal(HAlign horizontal) noexcept;

  // Changes the vertical alignment.
  Align& SetVertical(VAlign vertical) noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_ALIGN_H_
