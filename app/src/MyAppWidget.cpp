#include "MyAppWidget.h"
#include <neoflux/widgets/ButtonWidget.h>
#include <neoflux/widgets/ContainerWidget.h>
#include <neoflux/widgets/TextWidget.h>
#include <glog/logging.h>

namespace {
void onButtonPressed() {
  LOG(INFO) << "MyAppWidget: button pressed";
}
} // namespace

std::shared_ptr<neoflux::core::Widget> createAppWidget() {
  auto root = std::make_shared<neoflux::widgets::ContainerWidget>(neoflux::widgets::ContainerWidget::Direction::Column);
  auto title = std::make_shared<neoflux::widgets::TextWidget>("Welcome to NeoFlux");
  auto button = std::make_shared<neoflux::widgets::ButtonWidget>("Click Me");
  button->setOnClick(onButtonPressed);
  root->addChild(title);
  root->addChild(button);
  return root;
}
