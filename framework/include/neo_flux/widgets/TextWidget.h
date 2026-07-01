#pragma once

#include "../core/Widget.h"

#include <string>

namespace neoflux {
namespace widgets {

class TextWidget : public core::Widget {
 public:
  explicit TextWidget(std::string text = "Text");
  void setText(std::string text);
  void render() const override;

 private:
  std::string text_;
};

} // namespace widgets
} // namespace neoflux
