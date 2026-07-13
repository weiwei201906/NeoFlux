#include "MyAppWidget.h"

#include <neoflux/widgets/ButtonWidget.h>
#include <neoflux/widgets/ContainerWidget.h>
#include <neoflux/widgets/TextWidget.h>

std::shared_ptr<neoflux::core::Widget> createAppWidget() {
  using neoflux::widgets::ButtonWidget;
  using neoflux::widgets::ContainerWidget;
  using neoflux::widgets::TextWidget;

  auto root = std::make_shared<ContainerWidget>(ContainerWidget::Direction::Column);
  root->emplaceChild<TextWidget>("Hello, NeoFlux!");
  root->emplaceChild<TextWidget>("Skia pipeline demo");
  auto button = root->emplaceChild<ButtonWidget>("Render now");
  button->setOnClick([] { std::cout << "Button clicked" << '\n'; });
  return root;
}
