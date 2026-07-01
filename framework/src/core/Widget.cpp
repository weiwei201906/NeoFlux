#include "neoflux/core/Widget.h"

#include <utility>

namespace neoflux {
namespace core {

Widget::Widget(std::string_view name) : name_(name) {}
Widget::~Widget() = default;

void Widget::setBounds(int x, int y, int width, int height) {
  x_ = x;
  y_ = y;
  width_ = width;
  height_ = height;
}

void Widget::layout(const BuildContext& context) {
  for (auto& child : children_) {
    child->setBounds(x_, y_, width_, height_);
    child->layout(context);
  }
}

void Widget::render() const {
  for (const auto& child : children_) {
    child->render();
  }
}

void Widget::addChild(std::shared_ptr<Widget> child) {
  if (child != nullptr) {
    children_.push_back(std::move(child));
  }
}

const std::vector<std::shared_ptr<Widget>>& Widget::children() const {
  return children_;
}

const std::string& Widget::name() const {
  return name_;
}

int Widget::x() const {
  return x_;
}

int Widget::y() const {
  return y_;
}

int Widget::width() const {
  return width_;
}

int Widget::height() const {
  return height_;
}

}  // namespace core
}  // namespace neoflux
