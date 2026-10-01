// =============================================================================
// NeoFlux - padding.h
//
// A single-child widget that insets its child by a fixed EdgeInsets. This is a
// thin, purpose-built convenience over Container::SetPadding; the padding and
// child are supplied up front.
//
// All method implementations are in src/widget/padding.cpp.
// =============================================================================

#ifndef NEOFLUX_WIDGET_PADDING_H_
#define NEOFLUX_WIDGET_PADDING_H_

#include <memory>

#include "neoflux/core/types.h"
#include "neoflux/widget/container.h"

namespace neoflux {

// Insets its single child by the given edge insets.
class Padding : public Container {
 public:
  // Applies a uniform padding on all four edges around the child.
  explicit Padding(float all, std::shared_ptr<Widget> child = nullptr);

  // Applies explicit edge insets around the child.
  Padding(const EdgeInsets& insets, std::shared_ptr<Widget> child = nullptr);

  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_PADDING_H_
