#include <neoflux/widgets/ButtonWidget.h>

#include <glog/logging.h>
#include <utility>

namespace neoflux::widgets {

ButtonWidget::ButtonWidget(std::string_view title) : core::Widget(title), title_(title) {}

void ButtonWidget::setOnClick(std::function<void()> callback) { onClick_ = std::move(callback); }

const std::string& ButtonWidget::title() const { return title_; }

void ButtonWidget::layout(const core::BuildContext& context) {
  Widget::layout(context);
  LOG(INFO) << "ButtonWidget: layout for '" << title_ << "'";
}

void ButtonWidget::render() const {
  LOG(INFO) << "ButtonWidget: render '" << title_ << "'";
  if (onClick_) {
    onClick_();
  }
}

void ButtonWidget::render(const neoflux::render::RenderContext& context) const {
  if (context.out != nullptr) {
    *context.out << "[button] " << title_ << "\n";
  }
}

}  // namespace neoflux::widgets