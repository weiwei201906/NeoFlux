// =============================================================================
// NeoFlux - center.h
//
// A single-child widget that expands to fill its parent and centers its child
// on both axes. Implemented as a Container with flex-grow=1 and centered
// justify/align.
//
// Pimpl: Center::Impl (center.cpp) holds implementation state. This header
// includes nothing but <memory> and the widget base.
// =============================================================================

#ifndef NEOFLUX_WIDGET_CENTER_H_
#define NEOFLUX_WIDGET_CENTER_H_

#include <memory>

#include "neoflux/widgets/container.h"

namespace neoflux {

// Centers its child within the space it is given.
class Center : public Container {
 public:
  explicit Center(std::shared_ptr<Widget> child = nullptr);
  ~Center() override;

  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_CENTER_H_
