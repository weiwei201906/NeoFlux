// =============================================================================
// NeoFlux - text.h
//
// Text widget: displays a string of text with configurable style.
// All method implementations are in text.cpp.
// =============================================================================

#ifndef NEOFLUX_WIDGET_TEXT_H_
#define NEOFLUX_WIDGET_TEXT_H_

#include <string>
#include <string_view>

#include "neoflux/core/types.h"
#include "neoflux/widget/widget.h"

namespace neoflux {

// A widget that displays a single run of styled text.
class Text : public Widget {
 public:
  explicit Text(std::string text);
  ~Text() override;

  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;

  // Sets the text content.
  Text& SetText(std::string text);

  // Returns the text content.
  [[nodiscard]] std::string_view GetText() const noexcept;

  // Sets the text color.
  Text& SetTextColor(const Color& color) noexcept;

  // Sets the font size in pixels.
  Text& SetFontSize(float size) noexcept;

  // Sets the text alignment.
  Text& SetAlignment(HAlign align) noexcept;

  Size Layout(const LayoutConstraints& constraints) override;
  void Paint(RenderContext& context) override;

 private:
  std::string text_;
  Color text_color_;
  float font_size_;
  HAlign alignment_;
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_TEXT_H_
