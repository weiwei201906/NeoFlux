#include <neoflux/widgets/ContainerWidget.h>

#include <algorithm>
#include <glog/logging.h>

namespace neoflux {
namespace widgets {

ContainerWidget::ContainerWidget(Direction direction) : Widget("container"), direction_(direction) {}

void ContainerWidget::layout(const core::BuildContext& context) {
  const int childCount = static_cast<int>(children().size());
  if (childCount <= 0) {
    LOG(INFO) << "ContainerWidget: no children to layout";
    return;
  }

  const int spacing = 6;
  if (direction_ == Direction::Row) {
    const int unitWidth = std::max(1, (width_ - spacing * (childCount - 1)) / childCount);
    int cursorX = x_;
    for (int index = 0; index < childCount; ++index) {
      auto& child = children()[index];
      child->setBounds(cursorX, y_, unitWidth, height_);
      child->layout(context);
      cursorX += unitWidth + spacing;
    }
  } else {
    const int unitHeight = std::max(1, (height_ - spacing * (childCount - 1)) / childCount);
    int cursorY = y_;
    for (int index = 0; index < childCount; ++index) {
      auto& child = children()[index];
      child->setBounds(x_, cursorY, width_, unitHeight);
      child->layout(context);
      cursorY += unitHeight + spacing;
    }
  }
  LOG(INFO) << "ContainerWidget: layout with direction="
            << (direction_ == Direction::Row ? "row" : "column");
}

void ContainerWidget::render() const {
  LOG(INFO) << "ContainerWidget: render";
  Widget::render();
}

}  // namespace widgets
}  // namespace neoflux
