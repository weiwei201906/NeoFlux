#include "gtest/gtest.h"
#include <filesystem>
#include <neoflux/core/BuildContext.h>
#include <neoflux/core/Widget.h>
#include <neoflux/engine/RenderEngine.h>
#include <neoflux/platform/PlatformWindow.h>
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


TEST(NeoFluxPlaceholder, WidgetTreeIsCreated) {
  auto root = createAppWidget();
  ASSERT_NE(root, nullptr);
  EXPECT_FALSE(root->children().empty());
}

TEST(NeoFluxPlaceholder, LayoutComputesBounds) {
  neoflux::core::BuildContext context;
  context.width = 640;
  context.height = 480;

  auto root = createAppWidget();
  root->setBounds(0, 0, context.width, context.height);
  root->layout(context);

  EXPECT_EQ(root->width(), context.width);
  EXPECT_EQ(root->height(), context.height);
}

TEST(NeoFluxPlaceholder, EmplaceChildKeepsOwnershipAndBuilding) {
  auto root = std::make_shared<neoflux::widgets::ContainerWidget>(
      neoflux::widgets::ContainerWidget::Direction::Column);
  auto title = root->emplaceChild<neoflux::widgets::TextWidget>("Hello");
  auto button = root->emplaceChild<neoflux::widgets::ButtonWidget>("Click");

  ASSERT_EQ(root->children().size(), 2U);
  EXPECT_NE(title, nullptr);
  EXPECT_NE(button, nullptr);
  EXPECT_EQ(title->name(), "Hello");
}


TEST(NeoFluxPlaceholder, PlatformWindowCanOpenAndClose) {
  neoflux::platform::PlatformWindow window{"NeoFlux Test Window"};

  window.open();
  EXPECT_TRUE(window.isOpen());

  window.close();
  EXPECT_FALSE(window.isOpen());
}
