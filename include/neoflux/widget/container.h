// =============================================================================
// NeoFlux - container.h
//
// Container widget: a flexbox layout container backed by Taitank. Supports
// flex direction, padding, margin, justification, alignment, and fixed
// sizing. All layout is computed by Taitank; the Container only paints its
// background and delegates children to the base class.
// =============================================================================

#ifndef NEOFLUX_WIDGET_CONTAINER_H_
#define NEOFLUX_WIDGET_CONTAINER_H_

#include <memory>
#include <string_view>

#include "neoflux/core/types.h"
#include "neoflux/widget/widget.h"

namespace neoflux {

// Flex direction for Container layout.
enum class FlexDirection : std::uint8_t {
  kRow,         // Main axis horizontal, left to right.
  kRowReverse,  // Main axis horizontal, right to left.
  kColumn,      // Main axis vertical, top to bottom.
  kColumnReverse,  // Main axis vertical, bottom to top.
};

// A widget that lays out its children using Taitank flexbox.
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

  // Sets a fixed width (0 = flexible, determined by layout).
  Container& SetWidth(float width) noexcept;

  // Sets a fixed height (0 = flexible, determined by layout).
  Container& SetHeight(float height) noexcept;

  // Sets the child widget (convenience for single-child containers).
  Container& SetChild(std::shared_ptr<Widget> child);

  // Sets the flex direction (default: kColumn).
  Container& SetFlexDirection(FlexDirection direction) noexcept;

  // Sets justify content along the main axis.
  Container& SetJustifyContent(HAlign align) noexcept;

  // Sets align items along the cross axis.
  Container& SetAlignItems(VAlign align) noexcept;

  // Sets the flex grow factor (how much this container expands to fill
  // available space in its parent).
  Container& SetFlexGrow(float grow) noexcept;

  void Paint(RenderContext& context) override;

 private:
  void ApplyPaddingToTaitank() noexcept;
  void ApplyMarginToTaitank() noexcept;

  Color background_color_{};
  EdgeInsets padding_{};
  EdgeInsets margin_{};
  float fixed_width_ = 0.0F;
  float fixed_height_ = 0.0F;
  bool has_background_ = false;
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_CONTAINER_H_
