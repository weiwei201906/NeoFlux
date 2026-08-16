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
#include <string_view>

#include <neoflux/core/types.h>

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

  EXPECT_EQ(parent->GetChildCount(), 2U);
  EXPECT_EQ(child1->GetParent(), parent.get());
  EXPECT_EQ(child2->GetParent(), parent.get());
}

TEST(WidgetTest, ClearChildren) {
  auto parent = std::make_shared<TestWidget>();
  parent->AddChild(std::make_shared<TestWidget>());
  parent->AddChild(std::make_shared<TestWidget>());

  parent->ClearChildren();
  EXPECT_EQ(parent->GetChildCount(), 0U);
}

TEST(WidgetTest, BoundsAccessors) {
  TestWidget widget;
  Rect bounds{.x = 10.0F, .y = 20.0F, .width = 100.0F, .height = 50.0F};
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

  container.PerformLayout(800.0F, 600.0F);

  const auto& bounds = container.GetBounds();
  EXPECT_FLOAT_EQ(bounds.width, 200.0F);
  EXPECT_FLOAT_EQ(bounds.height, 100.0F);
}

TEST(ContainerTest, LayoutWrapsChild) {
  auto container = std::make_shared<Container>();
  auto text = std::make_shared<Text>("Hello");
  text->SetFontSize(16.0F);
  container->SetChild(text);
  container->SetPadding({5.0F, 5.0F, 5.0F, 5.0F});

  container->PerformLayout(800.0F, 600.0F);

  const auto& bounds = container->GetBounds();
  EXPECT_GT(bounds.width, 10.0F);
  EXPECT_GT(bounds.height, 10.0F);
}

TEST(TextTest, OnMeasureProducesPositiveSize) {
  Text text("Hello World");
  text.SetFontSize(20.0F);

  // Measure mode 0 = undefined (no constraint).
  const Size size = text.OnMeasure(0.0F, 0, 0.0F, 0);
  EXPECT_GT(size.width, 0.0F);
  EXPECT_GT(size.height, 0.0F);
}

TEST(ButtonTest, OnMeasureProducesPositiveSize) {
  Button button("Click Me");
  button.SetFontSize(16.0F);

  const Size size = button.OnMeasure(0.0F, 0, 0.0F, 0);
  EXPECT_GT(size.width, 0.0F);
  EXPECT_GT(size.height, 0.0F);
}

TEST(ButtonTest, PressAndRelease) {
  Button button("OK");
  button.SetBounds(
      Rect{.x = 0.0F, .y = 0.0F, .width = 100.0F, .height = 40.0F});

  bool pressed = false;
  button.SetOnPressed([&] { pressed = true; });

  EXPECT_TRUE(
      button.HandlePress(Point{.x = 50.0F, .y = 20.0F}));
  button.HandleRelease(Point{.x = 50.0F, .y = 20.0F});
  EXPECT_TRUE(pressed);
}

TEST(ButtonTest, PressOutsideDoesNothing) {
  Button button("OK");
  button.SetBounds(
      Rect{.x = 0.0F, .y = 0.0F, .width = 100.0F, .height = 40.0F});

  bool pressed = false;
  button.SetOnPressed([&] { pressed = true; });

  EXPECT_FALSE(
      button.HandlePress(Point{.x = 150.0F, .y = 200.0F}));
  button.HandleRelease(Point{.x = 150.0F, .y = 200.0F});
  EXPECT_FALSE(pressed);
}

}  // namespace
}  // namespace neoflux
