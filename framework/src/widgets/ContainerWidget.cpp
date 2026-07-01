#include <neoflux/widgets/ContainerWidget.h>

#include <glog/logging.h>

namespace neoflux {
namespace widgets {

ContainerWidget::ContainerWidget(Direction direction) : direction_(direction) {}

void ContainerWidget::layout(const core::BuildContext& context) {
  Widget::layout(context);
  LOG(INFO) << "ContainerWidget: layout with direction="
            << (direction_ == Direction::Row ? "row" : "column");
}

void ContainerWidget::render() const {
  LOG(INFO) << "ContainerWidget: render";
  Widget::render();
}

}  // namespace widgets
}  // namespace neoflux
