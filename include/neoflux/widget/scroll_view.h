// =============================================================================
// NeoFlux - scroll_view.h
//
// A scrollable container widget. The content is laid out by Taitank at its
// natural size; if it exceeds the viewport, the user can scroll via mouse
// wheel (desktop) or touch drag (mobile). Clipping is applied to the
// viewport bounds during painting.
// =============================================================================

#ifndef NEOFLUX_WIDGET_SCROLL_VIEW_H_
#define NEOFLUX_WIDGET_SCROLL_VIEW_H_

#include <memory>
#include <string_view>

#include "neoflux/core/types.h"
#include "neoflux/widget/widget.h"

namespace neoflux {

// A scrollable viewport that clips and scrolls its content.
//
// Usage:
//   auto scroll = std::make_shared<ScrollView>();
//   scroll->SetContent(big_column);
//   parent->AddChild(scroll);
class ScrollView : public Widget {
 public:
  ScrollView();
  ~ScrollView() override;

  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;

  // Sets the content widget. Replaces any existing content.
  void SetContent(std::shared_ptr<Widget> content);

  // Sets the scroll offset in pixels (clamped to valid range).
  void ScrollTo(float x, float y) noexcept;

  // Returns the current scroll offset.
  [[nodiscard]] Point GetScrollOffset() const noexcept;

  // Returns the content size (after layout).
  [[nodiscard]] Size GetContentSize() const noexcept;

  void Paint(RenderContext& context) override;

  // ScrollView has no intrinsic size; fills available space.
  [[nodiscard]] Size OnMeasure(float width, int width_mode, float height,
                               int height_mode) override;

  // Consumes scroll events to pan the content.
  bool OnPointerScroll(const Point& local_pos, double xoffset,
                       double yoffset) override;

 protected:
  void ReadLayoutRecursive() override;

 private:
  // Clamps scroll_offset_ to the valid range based on content/viewport size.
  void ClampScroll() noexcept;

  float scroll_x_ = 0.0F;
  float scroll_y_ = 0.0F;
  float content_width_ = 0.0F;
  float content_height_ = 0.0F;
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_SCROLL_VIEW_H_
