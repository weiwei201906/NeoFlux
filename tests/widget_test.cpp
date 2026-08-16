// =============================================================================
// NeoFlux - widget_test.cpp
//
// Unit tests for the widget system.
// =============================================================================

#include <neoflux/widget/button.h>
#include <neoflux/widget/container.h>
#include <neoflux/widget/text.h>
#include <neoflux/widget/widget.h>

#include <memory>

#include <gtest/gtest.h>

namespace neoflux {
namespace {

// A concrete widget for testing base class behavior.
class TestWidget : public Widget {
 public:
  [[nodiscard]] std::string_view GetWidgetName() const noexcept override {
    return "TestWidget";
  }

  int paint_count = 0;

  void Paint(RenderContext& /*context*/) override { ++paint_count; }
};

TEST(WidgetTest, AddAndCountChildren) {
  auto parent = std::make_shared<TestWidget>();
  auto child1 = std::make_shared<TestWidget>();
  auto child2 = std::make_shared<TestWidget>();

  parent->AddChild(child1);
  parent->AddChild(child2);

  EXPECT_EQ(parent->GetChildCount(), 2u);
  EXPECT_EQ(child1->GetParent(), parent.get());
  EXPECT_EQ(child2->GetParent(), parent.get());
}

TEST(WidgetTest, ClearChildren) {
  auto parent = std::make_shared<TestWidget>();
  parent->AddChild(std::make_shared<TestWidget>());
  parent->AddChild(std::make_shared<TestWidget>());

  parent->ClearChildren();
  EXPECT_EQ(parent->GetChildCount(), 0u);
}

TEST(WidgetTest, BoundsAccessors) {
  TestWidget widget;
  Rect bounds{10.0F, 20.0F, 100.0F, 50.0F};
  widget.SetBounds(bounds);

  const auto& result = widget.GetBounds();
  EXPECT_FLOAT_EQ(result.x, 10.0F);
  EXPECT_FLOAT_EQ(result.y, 20.0F);
  EXPECT_FLOAT_EQ(result.width, 100.0F);
  EXPECT_FLOAT_EQ(result.height, 50.0F);
}

TEST(WidgetTest, NeedsBuildFlag) {
  TestWidget widget;
  EXPECT_TRUE(widget.NeedsBuild());
  widget.ClearNeedsBuild();
  EXPECT_FALSE(widget.NeedsBuild());
  widget.MarkNeedsBuild();
  EXPECT_TRUE(widget.NeedsBuild());
}

TEST(ContainerTest, LayoutWithFixedSize) {
  Container container;
  container.SetWidth(200.0F).SetHeight(100.0F);

  LayoutConstraints constraints;
  constraints.max_width = 800.0F;
  constraints.max_height = 600.0F;

  const Size size = container.Layout(constraints);
  EXPECT_FLOAT_EQ(size.width, 200.0F);
  EXPECT_FLOAT_EQ(size.height, 100.0F);
}

TEST(ContainerTest, LayoutWrapsChild) {
  auto container = std::make_shared<Container>();
  auto text = std::make_shared<Text>("Hello");
  text->SetFontSize(16.0F);
  container->SetChild(text);
  container->SetPadding({5.0F, 5.0F, 5.0F, 5.0F});

  LayoutConstraints constraints;
  constraints.max_width = 800.0F;
  constraints.max_height = 600.0F;

  const Size size = container->Layout(constraints);
  EXPECT_GT(size.width, 10.0F);
  EXPECT_GT(size.height, 10.0F);
}

TEST(TextTest, LayoutProducesPositiveSize) {
  Text text("Hello World");
  text.SetFontSize(20.0F);

  LayoutConstraints constraints;
  constraints.max_width = 800.0F;
  constraints.max_height = 600.0F;

  const Size size = text.Layout(constraints);
  EXPECT_GT(size.width, 0.0F);
  EXPECT_GT(size.height, 0.0F);
}

TEST(ButtonTest, LayoutProducesPositiveSize) {
  Button button("Click Me");
  button.SetFontSize(16.0F);

  LayoutConstraints constraints;
  constraints.max_width = 800.0F;
  constraints.max_height = 600.0F;

  const Size size = button.Layout(constraints);
  EXPECT_GT(size.width, 0.0F);
  EXPECT_GT(size.height, 0.0F);
}

TEST(ButtonTest, PressAndRelease) {
  Button button("OK");
  button.SetBounds({0.0F, 0.0F, 100.0F, 40.0F});

  bool pressed = false;
  button.SetOnPressed([&]() { pressed = true; });

  EXPECT_TRUE(button.HandlePress({50.0F, 20.0F}));
  button.HandleRelease({50.0F, 20.0F});
  EXPECT_TRUE(pressed);
}

TEST(ButtonTest, PressOutsideDoesNothing) {
  Button button("OK");
  button.SetBounds({0.0F, 0.0F, 100.0F, 40.0F});

  bool pressed = false;
  button.SetOnPressed([&]() { pressed = true; });

  EXPECT_FALSE(button.HandlePress({150.0F, 200.0F}));
  button.HandleRelease({150.0F, 200.0F});
  EXPECT_FALSE(pressed);
}

}  // namespace
}  // namespace neoflux
