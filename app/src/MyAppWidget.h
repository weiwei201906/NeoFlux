#pragma once

#include <memory>

#include <neoflux/widgets/ContainerWidget.h>
#include <neoflux/widgets/TextWidget.h>
#include <neoflux/widgets/ButtonWidget.h>

// Simple app widget used by tests and demo app.
static inline std::shared_ptr<neoflux::core::Widget> createAppWidget() {
  using neoflux::widgets::ContainerWidget;
  using neoflux::widgets::TextWidget;
  using neoflux::widgets::ButtonWidget;

  auto root = std::make_shared<ContainerWidget>(ContainerWidget::Direction::Column);
  auto title = root->emplaceChild<TextWidget>("Hello, NeoFlux!");
  auto subtitle = root->emplaceChild<TextWidget>("Skia pipeline demo");
  auto button = root->emplaceChild<ButtonWidget>("Render now");
  button->setOnClick([] { /* noop for tests */ });
  (void)title;
  (void)subtitle;
  return root;
}
