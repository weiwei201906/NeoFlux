// =============================================================================
// NeoFlux - sized_box.h
//
// A widget with fixed dimensions. Useful for spacing, dividers, or
// reserving a known amount of space in a flex layout.
//
// All method implementations are in src/widget/sized_box.cpp.
// =============================================================================

#ifndef NEOFLUX_WIDGET_SIZED_BOX_H_
#define NEOFLUX_WIDGET_SIZED_BOX_H_

#include "neoflux/widget/container.h"

namespace neoflux {

// A widget with fixed width and/or height. If a dimension is 0 or negative,
// it is left unconstrained (Taitank auto-sizes that axis).
class SizedBox : public Container {
 public:
  SizedBox();
  explicit SizedBox(float width, float height = 0.0F);

  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;

  // Sets the fixed width. Returns *this for chaining.
  SizedBox& SetWidth(float width);

  // Sets the fixed height. Returns *this for chaining.
  SizedBox& SetHeight(float height);

  // Sets both dimensions. Returns *this for chaining.
  SizedBox& SetSize(float width, float height);
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_SIZED_BOX_H_
