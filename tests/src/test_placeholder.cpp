#include "gtest/gtest.h"
#include <neoflux/core/BuildContext.h>
#include <neoflux/core/Widget.h>
#include "MyAppWidget.h"

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
