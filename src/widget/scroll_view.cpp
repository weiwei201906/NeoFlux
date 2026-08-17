// =============================================================================
// NeoFlux - scroll_view.cpp
//
// Implementation of the ScrollView widget. Content is laid out by Taitank
// at its natural size; painting applies a clip rect and a translate offset
// to implement scrolling.
// =============================================================================

#include "neoflux/widget/scroll_view.h"

#include <algorithm>

#include <glog/logging.h>

#include "taitank.h"

#include "neoflux/render/render_context.h"

namespace neoflux {

ScrollView::ScrollView() {
  auto* node = GetTaitankNode();
  if (node != nullptr) {
    // Fill available space in the parent by default. Flex shrink must be
    // non-zero so the viewport is clamped to the parent's remaining space
    // instead of expanding to the content's natural height.
    taitank::SetFlexGrow(node, 1.0F);
    taitank::SetFlexShrink(node, 1.0F);
    taitank::SetOverflow(node, taitank::OVERFLOW_HIDDEN);
  }
}

ScrollView::~ScrollView() = default;

std::string_view ScrollView::GetWidgetName() const noexcept {
  return "ScrollView";
}

void ScrollView::SetContent(std::shared_ptr<Widget> content) {
  ClearChildren();
  if (content != nullptr) {
    auto* node = content->GetTaitankNode();
    if (node != nullptr) {
      // Prevent content from being shrunk to fit the viewport; it should
      // keep its natural size and be clipped/scrollable instead.
      taitank::SetFlexShrink(node, 0.0F);
      taitank::SetAlignSelf(node, taitank::FLEX_ALIGN_START);
    }
    AddChild(std::move(content));
  }
}

void ScrollView::ScrollTo(float x, float y) noexcept {
  scroll_x_ = x;
  scroll_y_ = y;
  ClampScroll();
}

Point ScrollView::GetScrollOffset() const noexcept {
  return {.x = scroll_x_, .y = scroll_y_};
}

Size ScrollView::GetContentSize() const noexcept {
  return {.width = content_width_, .height = content_height_};
}

void ScrollView::Paint(RenderContext& context) {
  context.Save();
  // Clip to the viewport bounds so content outside is not visible.
  context.ClipRect(bounds_);
  // Translate content by the negative scroll offset.
  context.Translate(-scroll_x_, -scroll_y_);
  PaintChildren(context);
  context.Restore();
}

Size ScrollView::OnMeasure(float /*width*/, int /*width_mode*/,
                           float /*height*/, int /*height_mode*/) {
  return {.width = 0.0F, .height = 0.0F};
}

bool ScrollView::OnPointerScroll(const Point& /*local_pos*/, double xoffset,
                                 double yoffset) {
  // Scroll speed: 32 pixels per wheel notch.
  constexpr float kScrollSpeed = 32.0F;
  scroll_x_ -= static_cast<float>(xoffset) * kScrollSpeed;
  scroll_y_ -= static_cast<float>(yoffset) * kScrollSpeed;
  ClampScroll();
  return true;
}

void ScrollView::ReadLayoutRecursive() {
  Widget::ReadLayoutRecursive();
  // Compute content size from the first (and only) child after layout.
  content_width_ = 0.0F;
  content_height_ = 0.0F;
  const auto& children = GetChildren();
  if (!children.empty() && children.front() != nullptr) {
    const Rect& cb = children.front()->GetBounds();
    content_width_ = cb.x + cb.width;
    content_height_ = cb.y + cb.height;
  }
  ClampScroll();
}

void ScrollView::ClampScroll() noexcept {
  const float max_x = std::max(0.0F, content_width_ - bounds_.width);
  const float max_y = std::max(0.0F, content_height_ - bounds_.height);
  scroll_x_ = std::clamp(scroll_x_, 0.0F, max_x);
  scroll_y_ = std::clamp(scroll_y_, 0.0F, max_y);
}

}  // namespace neoflux
