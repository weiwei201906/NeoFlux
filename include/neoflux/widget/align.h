// =============================================================================
// NeoFlux - align.h
//
// A single-child widget that expands to fill its parent and positions its child
// at a chosen horizontal/vertical alignment (e.g. top-left, bottom-right).
// More general than Center, which is equivalent to Align(kCenter, kCenter).
//
// All method implementations are in src/widget/align.cpp.
// =============================================================================

#ifndef NEOFLUX_WIDGET_ALIGN_H_
#define NEOFLUX_WIDGET_ALIGN_H_

#include <memory>

#include "neoflux/core/types.h"
#include "neoflux/widget/container.h"

namespace neoflux {

// Positions its child within the space it is given.
class Align : public Container {
 public:
  Align(HAlign horizontal, VAlign vertical,
        std::shared_ptr<Widget> child = nullptr);

  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;

  // Changes the horizontal alignment.
  Align& SetHorizontal(HAlign horizontal) noexcept;

  // Changes the vertical alignment.
  Align& SetVertical(VAlign vertical) noexcept;

 private:
  // Pushes the current alignment onto the Taitank node.
  void ApplyAlignment() noexcept;

  HAlign horizontal_ = HAlign::kCenter;
  VAlign vertical_ = VAlign::kCenter;
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_ALIGN_H_
