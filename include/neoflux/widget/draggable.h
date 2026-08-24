// =============================================================================
// NeoFlux - draggable.h
//
// A container widget that can be dragged with the pointer. Demonstrates the
// widget state machine (kIdle <-> kDragging) integrated with pointer move
// events. The drag offset is applied as a paint-time translate, so Taitank
// layout is unaffected.
// =============================================================================

#ifndef NEOFLUX_WIDGET_DRAGGABLE_H_
#define NEOFLUX_WIDGET_DRAGGABLE_H_

#include <memory>

#include "neoflux/core/types.h"
#include "neoflux/widget/container.h"

namespace neoflux {

// A container whose children can be dragged by the pointer.
//
// States:
//   kIdle     - not being dragged
//   kDragging - pointer is down and moving the widget
//
// Usage:
//   auto drag = std::make_shared<Draggable>();
//   drag->AddChild(box);
//   parent->AddChild(drag);
class Draggable : public Container {
 public:
  Draggable();
  ~Draggable() override;

  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;

  // Paints children translated by the current drag offset.
  void Paint(RenderContext& context) override;

  // Hit test with the drag offset applied. Since the visual position is
  // offset by drag_offset_ at paint time, the hit test must subtract the
  // offset so clicks at the visual position correctly hit the widget.
  [[nodiscard]] std::shared_ptr<Widget> HitTest(
      const Point& parent_pos) override;

  // Pointer handlers that drive the drag state machine.
  bool OnPointerDown(const Point& local_pos) override;
  void OnPointerUp(const Point& local_pos) override;
  bool OnPointerMove(const Point& local_pos) override;
  void OnPointerEnter() override;
  void OnPointerExit() override;

  // Returns the current drag offset in pixels.
  [[nodiscard]] Point GetDragOffset() const noexcept;

  // Returns true if currently being dragged.
  [[nodiscard]] bool IsDragging() const noexcept;

 private:
  Point drag_offset_{};
  Point press_pos_{};
  bool dragging_ = false;
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_DRAGGABLE_H_
