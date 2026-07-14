#include <neoflux/widgets/ContainerWidget.h>

#include <algorithm>
#include <glog/logging.h>

namespace neoflux::widgets {

ContainerWidget::ContainerWidget(Direction direction) : Widget("container"), direction_(direction) {}

void ContainerWidget::layout(const core::BuildContext& context) {
  const std::size_t childCount = children().size();
  if (childCount == 0U) {
    LOG(INFO) << "ContainerWidget: no children to layout";
    return;
  }

  constexpr int spacing = 6;
  std::size_t totalFlex = 0U;
  for (const auto& child : children()) {
    totalFlex += std::max<std::size_t>(1U, child->flex());
  }

  if (direction_ == Direction::Row) {
    const int availableWidth = std::max(0, width_ - spacing * static_cast<int>(childCount - 1));
    int cursorX = x_;
    for (std::size_t index = 0; index < childCount; ++index) {
      const auto& child = children()[index];
      const int childWidth =
          totalFlex == 0U ? 1
                          : std::max(1, availableWidth * static_cast<int>(std::max<std::size_t>(1U, child->flex())) /
                                            static_cast<int>(totalFlex));
      child->setBounds(cursorX, y_, childWidth, height_);
      child->layout(context);
      cursorX += childWidth + spacing;
    }
  } else {
    const int availableHeight = std::max(0, height_ - spacing * static_cast<int>(childCount - 1));
    int cursorY = y_;
    for (std::size_t index = 0; index < childCount; ++index) {
      const auto& child = children()[index];
      const int childHeight =
          totalFlex == 0U ? 1
                          : std::max(1, availableHeight * static_cast<int>(std::max<std::size_t>(1U, child->flex())) /
                                            static_cast<int>(totalFlex));
      child->setBounds(x_, cursorY, width_, childHeight);
      child->layout(context);
      cursorY += childHeight + spacing;
    }
  }
  LOG(INFO) << "ContainerWidget: layout with direction=" << (direction_ == Direction::Row ? "row" : "column");
}

void ContainerWidget::render() const {
  LOG(INFO) << "ContainerWidget: render";
  Widget::render();
}

void ContainerWidget::render(const neoflux::render::RenderContext& context) const {
  if (context.out != nullptr) {
    *context.out << "[container] direction=" << (direction_ == Direction::Row ? "row" : "column") << "\n";
  }
  for (const auto& child : children()) {
    child->render(context);
  }
}

}  // namespace neoflux::widgets