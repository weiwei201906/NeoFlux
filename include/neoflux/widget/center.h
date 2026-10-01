// =============================================================================
// NeoFlux - center.h
//
// A single-child widget that expands to fill its parent and centers its child
// on both axes. Implemented as a Container with flex-grow=1 and centered
// justify/align.
//
// All method implementations are in src/widget/center.cpp.
// =============================================================================

#ifndef NEOFLUX_WIDGET_CENTER_H_
#define NEOFLUX_WIDGET_CENTER_H_

#include <memory>

#include "neoflux/widget/container.h"

namespace neoflux {

// Centers its child within the space it is given.
class Center : public Container {
 public:
  explicit Center(std::shared_ptr<Widget> child = nullptr);

  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_CENTER_H_
