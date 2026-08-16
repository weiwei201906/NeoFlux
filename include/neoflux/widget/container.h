// =============================================================================
// NeoFlux - container.h
//
// Container widget: combines common painting, positioning, and sizing of
// a single child. All method implementations are in container.cpp.
// =============================================================================

#ifndef NEOFLUX_WIDGET_CONTAINER_H_
#define NEOFLUX_WIDGET_CONTAINER_H_

#include <memory>
#include <string_view>

#include "neoflux/core/types.h"
#include "neoflux/widget/widget.h"

namespace neoflux {

// A widget that contains a single child with optional styling and sizing.
class Container : public Widget {
 public:
  Container();
  ~Container() override;

  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;

  // Sets the background color.
  Container& SetBackgroundColor(const Color& color) noexcept;

  // Sets the padding inside the container.
  Container& SetPadding(const EdgeInsets& padding) noexcept;

  // Sets the margin outside the container.
  Container& SetMargin(const EdgeInsets& margin) noexcept;

  // Sets a fixed width (0 = unspecified, use child's width).
  Container& SetWidth(float width) noexcept;

  // Sets a fixed height (0 = unspecified, use child's height).
  Container& SetHeight(float height) noexcept;

  // Sets the child widget.
  Container& SetChild(std::shared_ptr<Widget> child);

  // Sets horizontal alignment of the child within the container.
  Container& SetAlignment(HAlign align) noexcept;

  Size Layout(const LayoutConstraints& constraints) override;
  void Paint(RenderContext& context) override;

 private:
  Color background_color_;
  EdgeInsets padding_;
  EdgeInsets margin_;
  float fixed_width_;
  float fixed_height_;
  HAlign alignment_;
  bool has_background_;
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_CONTAINER_H_
