// =============================================================================
// NeoFlux - expanded.h
//
// A widget that expands to fill available space along the main axis of its
// parent flex container. Equivalent to Container with flex_grow = 1.
//
// All method implementations are in src/widget/expanded.cpp.
// =============================================================================

#ifndef NEOFLUX_WIDGET_EXPANDED_H_
#define NEOFLUX_WIDGET_EXPANDED_H_

#include "neoflux/widget/container.h"

namespace neoflux {

// A widget that expands to fill the remaining space in a flex layout.
// The child widget is stretched to fill the Expanded's bounds.
class Expanded : public Container {
 public:
  explicit Expanded(std::shared_ptr<Widget> child = nullptr, int flex = 1);

  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;

  // Sets the flex factor. Higher values claim more space. Default: 1.
  Expanded& SetFlex(int flex);
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_EXPANDED_H_
