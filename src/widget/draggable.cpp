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

bool Draggable::OnPointerDown(const Point& local_pos) {
  press_pos_ = local_pos;
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
  drag_offset_.x = local_pos.x - press_pos_.x;
  drag_offset_.y = local_pos.y - press_pos_.y;
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

bool Draggable::IsDragging() const noexcept { return dragging_; }

}  // namespace neoflux
