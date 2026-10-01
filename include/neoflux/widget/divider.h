// =============================================================================
// NeoFlux - divider.h
//
// A thin horizontal or vertical line used to separate content. The divider is
// a zero-child Container that fills the main-axis extent of its parent and
// fixes its thickness on the cross axis; the line itself is painted as a solid
// background rect.
//
// All method implementations are in src/widget/divider.cpp.
// =============================================================================

#ifndef NEOFLUX_WIDGET_DIVIDER_H_
#define NEOFLUX_WIDGET_DIVIDER_H_

#include "neoflux/core/types.h"
#include "neoflux/widget/container.h"

namespace neoflux {

// Orientation of the divider line.
enum class DividerOrientation : std::uint8_t {
  kHorizontal,  // Line runs left-to-right; fixed thickness is the height.
  kVertical,    // Line runs top-to-bottom; fixed thickness is the width.
};

// A 1px-ish separator line.
class Divider : public Container {
 public:
  Divider();
  explicit Divider(float thickness);

  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;

  // Sets the line color.
  Divider& SetColor(const Color& color) noexcept;

  // Sets the line thickness in pixels (default: 1.0).
  Divider& SetThickness(float thickness) noexcept;

  // Sets the divider orientation (default: horizontal).
  Divider& SetOrientation(DividerOrientation orientation) noexcept;

 private:
  // Applies the fixed thickness to the Taitank node for the current axis.
  void ApplyThickness() noexcept;

  float thickness_ = 1.0F;
  DividerOrientation orientation_ = DividerOrientation::kHorizontal;
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_DIVIDER_H_
