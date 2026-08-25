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

std::shared_ptr<Widget> Draggable::HitTest(const Point& parent_pos) const {
  // The widget is visually translated by drag_offset_ at paint time.
  // Subtract the offset to convert the click position (in visual
  // coordinates) back to layout coordinates before delegating to the
  // base class hit test (which uses bounds_ in layout space).
  const Point layout_pos{.x = parent_pos.x - drag_offset_.x,
                         .y = parent_pos.y - drag_offset_.y,};
  return Container::HitTest(layout_pos);
}

bool Draggable::OnPointerDown(const Point& local_pos) {
  dragging_ = true;
  SetState(WidgetState::kDragging);
  // Immediately center the widget on the press point.
  // Multiply by 0.5 instead of divide by 2.0: FP multiply is ~4 cycles vs
  // divide ~14 cycles on most architectures.
  drag_offset_.x += local_pos.x - bounds_.width * 0.5F;
  drag_offset_.y += local_pos.y - bounds_.height * 0.5F;
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
  // Keep the widget centered on the cursor: local_pos is relative to the
  // current visual top-left, so adding (local_pos - size/2) to drag_offset_
  // moves the widget so its center lands on the cursor. Paint-time translate
  // only: no full rebuild, frame dirty is set by pointer dispatch.
  // Multiply by 0.5 instead of divide by 2.0 (FP multiply ~4c vs divide ~14c).
  drag_offset_.x += local_pos.x - bounds_.width * 0.5F;
  drag_offset_.y += local_pos.y - bounds_.height * 0.5F;
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
