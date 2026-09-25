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
// Supports both mouse-wheel scrolling and pointer-drag scrolling. The
// drag state machine (Idle/Dragging) is local to ScrollView; on release
// an inertia coroutine may be launched to decelerate the scroll.
//
// Usage:
//   auto scroll = std::make_shared<ScrollView>();
//   scroll->SetContent(big_column);
//   parent->AddChild(scroll);
class ScrollView : public Widget {
 public:
  // Scroll drag states. Kept local to ScrollView (not in Widget base) to
  // avoid polluting the base class with derived-specific state.
  enum class ScrollState : std::uint8_t { kIdle, kDragging };

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

  // Consumes scroll events to pan the content.
  bool OnPointerScroll(const Point& local_pos, double xoffset,
                       double yoffset) override;

  // Pointer-drag scrolling handlers.
  bool OnPointerDown(const Point& local_pos) override;
  void OnPointerUp(const Point& local_pos) override;
  bool OnPointerMove(const Point& local_pos) override;

 protected:
  void ReadLayoutRecursive() override;

 private:
  // Clamps scroll_offset_ to the valid range based on content/viewport size.
  void ClampScroll() noexcept;

  float scroll_x_ = 0.0F;
  float scroll_y_ = 0.0F;
  float content_width_ = 0.0F;
  float content_height_ = 0.0F;

  // Drag-scroll state.
  ScrollState scroll_state_ = ScrollState::kIdle;
  Point drag_start_pos_{};
  float drag_start_scroll_y_ = 0.0F;
  float last_move_y_ = 0.0F;
  float last_velocity_ = 0.0F;
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_SCROLL_VIEW_H_
