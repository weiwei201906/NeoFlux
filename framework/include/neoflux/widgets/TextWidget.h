#pragma once

#include "../core/Widget.h"

#include <string>
#include <string_view>

namespace neoflux {
namespace widgets {

class TextWidget : public core::Widget {
 public:
  explicit TextWidget(std::string_view text = "Text");
  void setText(std::string_view text);
  const std::string& text() const;
  void render() const override;

 private:
  std::string text_;
};

} // namespace widgets
} // namespace neoflux
