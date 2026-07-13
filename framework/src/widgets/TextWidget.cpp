#include <neoflux/widgets/TextWidget.h>

#include <glog/logging.h>

namespace neoflux {
namespace widgets {

TextWidget::TextWidget(std::string_view text) : core::Widget(text), text_(text) {}

void TextWidget::setText(std::string_view text) {
  text_ = text;
}

const std::string& TextWidget::text() const {
  return text_;
}

void TextWidget::render() const {
  LOG(INFO) << "TextWidget: " << text_;
}

}  // namespace widgets
}  // namespace neoflux
