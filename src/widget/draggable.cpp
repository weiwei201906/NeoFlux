// =============================================================================
// NeoFlux - draggable.cpp
//
// Implementation of the Draggable widget. Pointer-down starts a drag,
// pointer-move updates the offset, pointer-up ends the drag. The offset is
// applied as a paint-time translate so Taitank layout is unaffected.
// =============================================================================

#include "neoflux/widget/draggable.h"

#include <utility>

#include <glog/logging.h>

#include "neoflux/core/types.h"
#include "neoflux/render/render_context.h"

namespace neoflux {

Draggable::Draggable() = default;

Draggable::~Draggable() = default;

std::string_view Draggable::GetWidgetName() const noexcept {
  return "Draggable";
}

void Draggable::Paint(RenderContext& context) {
  context.Save();
  context.Translate(drag_offset_.x, drag_offset_.y);
  Container::Paint(context);
  context.Restore();
}

std::shared_ptr<Widget> Draggable::HitTest(const Point& parent_pos) {
  // The widget is visually translated by drag_offset_ at paint time.
  // Subtract the offset to convert the click position (in visual
  // coordinates) back to layout coordinates before delegating to the
  // base class hit test (which uses bounds_ in layout space).
  const Point layout_pos{.x = parent_pos.x - drag_offset_.x,
                         .y = parent_pos.y - drag_offset_.y,};
  return Container::HitTest(layout_pos);
}

bool Draggable::OnPointerDown(const Point& local_pos) {
  press_pos_ = local_pos;
  start_offset_ = drag_offset_;
  dragging_ = true;
  SetState(WidgetState::kDragging);
  return true;
}

void Draggable::OnPointerUp(const Point& /*local_pos*/) {
  if (dragging_) {
    dragging_ = false;
    SetState(WidgetState::kIdle);
  }
}

bool Draggable::OnPointerMove(const Point& local_pos) {
  if (!dragging_) {
    return false;
  }
  // Paint-time translate only: update offset without triggering a full
  // widget rebuild. The pointer event already marks the frame dirty, so
  // Layout+Paint will run with the new offset. This avoids flooding the
  // render queue with redundant full-tree rebuilds during high-frequency
  // drag events (Flutter-style repaint without relayout).
  //
  // Absolute offset = start_offset + (current_pos - press_pos), where both
  // positions are relative to the widget's visual top-left.
  drag_offset_.x = start_offset_.x + (local_pos.x - press_pos_.x);
  drag_offset_.y = start_offset_.y + (local_pos.y - press_pos_.y);
  return true;
}

void Draggable::OnPointerEnter() {
  if (!dragging_) {
    SetState(WidgetState::kHovering);
  }
}

void Draggable::OnPointerExit() {
  if (!dragging_) {
    SetState(WidgetState::kIdle);
  }
}

Point Draggable::GetDragOffset() const noexcept { return drag_offset_; }

Point Draggable::GetPaintOffset() const noexcept { return drag_offset_; }

bool Draggable::IsDragging() const noexcept { return dragging_; }

}  // namespace neoflux
