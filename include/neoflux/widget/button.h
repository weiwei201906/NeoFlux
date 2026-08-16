// =============================================================================
// NeoFlux - button.h
//
// Button widget: a clickable button with a label and optional callback.
// All method implementations are in button.cpp.
// =============================================================================

#ifndef NEOFLUX_WIDGET_BUTTON_H_
#define NEOFLUX_WIDGET_BUTTON_H_

#include <functional>
#include <string>
#include <string_view>

#include "neoflux/core/types.h"
#include "neoflux/widget/widget.h"

namespace neoflux {

// A clickable button widget.
class Button : public Widget {
 public:
  // Callback type for button press events.
  using OnPressed = std::function<void()>;

  explicit Button(std::string label);
  ~Button() override;

  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;

  // Sets the button label text.
  Button& SetLabel(std::string label);

  // Returns the button label.
  [[nodiscard]] std::string_view GetLabel() const noexcept;

  // Sets the callback invoked when the button is pressed.
  Button& SetOnPressed(OnPressed callback) noexcept;

  // Sets the button background color.
  Button& SetBackgroundColor(const Color& color) noexcept;

  // Sets the button text color.
  Button& SetTextColor(const Color& color) noexcept;

  // Sets the button font size.
  Button& SetFontSize(float size) noexcept;

  // Handles a press event at the given local coordinates.
  bool HandlePress(const Point& local_pos);

  // Handles a release event at the given local coordinates.
  void HandleRelease(const Point& local_pos);

  // Reports intrinsic button size to the Taitank layout engine.
  [[nodiscard]] Size OnMeasure(float width, int width_mode, float height,
                               int height_mode) override;

  void Paint(RenderContext& context) override;

 private:
  // Returns true if the given point is inside the button bounds.
  [[nodiscard]] bool ContainsPoint(const Point& point) const noexcept;

  std::string label_;
  OnPressed on_pressed_;
  Color background_color_;
  Color text_color_;
  Color pressed_color_;
  float font_size_;
  float horizontal_padding_;
  float vertical_padding_;
  bool is_pressed_;
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_BUTTON_H_
