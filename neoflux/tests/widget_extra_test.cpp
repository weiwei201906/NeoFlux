// =============================================================================
// NeoFlux - widget_extra_test.cpp
//
// Unit tests for the additional layout and interactive widgets: Divider,
// Padding, Center, Align, Card, ProgressIndicator, Switch, Checkbox.
// =============================================================================

#include <neoflux/widgets/align.h>
#include <neoflux/widgets/card.h>
#include <neoflux/widgets/center.h>
#include <neoflux/widgets/checkbox.h>
#include <neoflux/widgets/divider.h>
#include <neoflux/widgets/padding.h>
#include <neoflux/widgets/progress_indicator.h>
#include <neoflux/widgets/switch.h>
#include <neoflux/widgets/widget.h>

#include <memory>
#include <string_view>

#include <neoflux/core/types.h>
#include <neoflux/renderers/render_context.h>

#include <gtest/gtest.h>

namespace neoflux {
namespace {

// ---------------------------------------------------------------------------
// Divider
// ---------------------------------------------------------------------------

TEST(DividerTest, HorizontalThicknessIsPinned) {
  auto parent = std::make_shared<Container>();
  auto divider = std::make_shared<Divider>();
  divider->SetThickness(3.0F);
  parent->AddChild(divider);

  parent->PerformLayout(400.0F, 200.0F);

  // Horizontal divider stretches to the parent width and pins its height.
  const auto& bounds = divider->GetBounds();
  EXPECT_FLOAT_EQ(bounds.height, 3.0F);
  EXPECT_FLOAT_EQ(bounds.width, 400.0F);
}

TEST(DividerTest, VerticalThicknessIsPinnedToWidth) {
  // A vertical divider pins its width and stretches its height along the cross
  // axis of a ROW parent (align-items: stretch by default).
  auto parent = std::make_shared<Container>();
  parent->SetFlexDirection(FlexDirection::kRow);
  parent->SetWidth(200.0F).SetHeight(400.0F);
  auto divider = std::make_shared<Divider>();
  divider->SetOrientation(DividerOrientation::kVertical);
  divider->SetThickness(2.0F);
  parent->AddChild(divider);

  parent->PerformLayout(200.0F, 400.0F);

  const auto& bounds = divider->GetBounds();
  EXPECT_FLOAT_EQ(bounds.width, 2.0F);
  EXPECT_FLOAT_EQ(bounds.height, 400.0F);
}

// ---------------------------------------------------------------------------
// Padding
// ---------------------------------------------------------------------------

TEST(PaddingTest, ChildIsInsetByPadding) {
  auto parent = std::make_shared<Container>();
  auto child = std::make_shared<Container>();
  child->SetWidth(50.0F).SetHeight(50.0F);
  auto padding = std::make_shared<Padding>(10.0F, child);
  parent->AddChild(padding);

  parent->PerformLayout(200.0F, 200.0F);

  // The child must be offset by the 10px padding on both axes.
  const auto& child_bounds = child->GetBounds();
  EXPECT_FLOAT_EQ(child_bounds.x, 10.0F);
  EXPECT_FLOAT_EQ(child_bounds.y, 10.0F);
}

// ---------------------------------------------------------------------------
// Center
// ---------------------------------------------------------------------------

TEST(CenterTest, ChildIsCentered) {
  auto parent = std::make_shared<Container>();
  parent->SetWidth(400.0F).SetHeight(200.0F);
  auto child = std::make_shared<Container>();
  child->SetWidth(50.0F).SetHeight(50.0F);
  auto center = std::make_shared<Center>(child);
  parent->AddChild(center);

  parent->PerformLayout(400.0F, 200.0F);

  // Center fills the parent; the 50x50 child lands at ((400-50)/2, (200-50)/2).
  const auto& center_bounds = center->GetBounds();
  EXPECT_FLOAT_EQ(center_bounds.width, 400.0F);
  EXPECT_FLOAT_EQ(center_bounds.height, 200.0F);
  const auto& child_bounds = child->GetBounds();
  EXPECT_FLOAT_EQ(child_bounds.x, 175.0F);
  EXPECT_FLOAT_EQ(child_bounds.y, 75.0F);
}

// ---------------------------------------------------------------------------
// Align
// ---------------------------------------------------------------------------

TEST(AlignTest, TopLeftChildAtOrigin) {
  auto parent = std::make_shared<Container>();
  parent->SetWidth(300.0F).SetHeight(300.0F);
  auto child = std::make_shared<Container>();
  child->SetWidth(40.0F).SetHeight(40.0F);
  auto align = std::make_shared<Align>(HAlign::kLeft, VAlign::kTop, child);
  parent->AddChild(align);

  parent->PerformLayout(300.0F, 300.0F);

  const auto& child_bounds = child->GetBounds();
  EXPECT_FLOAT_EQ(child_bounds.x, 0.0F);
  EXPECT_FLOAT_EQ(child_bounds.y, 0.0F);
}

TEST(AlignTest, BottomRightChildAtFarCorner) {
  auto parent = std::make_shared<Container>();
  parent->SetWidth(300.0F).SetHeight(300.0F);
  auto child = std::make_shared<Container>();
  child->SetWidth(40.0F).SetHeight(40.0F);
  auto align = std::make_shared<Align>(HAlign::kRight, VAlign::kBottom, child);
  parent->AddChild(align);

  parent->PerformLayout(300.0F, 300.0F);

  const auto& child_bounds = child->GetBounds();
  EXPECT_FLOAT_EQ(child_bounds.x, 260.0F);
  EXPECT_FLOAT_EQ(child_bounds.y, 260.0F);
}

// ---------------------------------------------------------------------------
// Card
// ---------------------------------------------------------------------------

TEST(CardTest, ChildIsInsetByDefaultPadding) {
  auto parent = std::make_shared<Container>();
  parent->SetWidth(300.0F).SetHeight(200.0F);
  auto child = std::make_shared<Container>();
  child->SetWidth(40.0F).SetHeight(40.0F);
  auto card = std::make_shared<Card>(child);
  parent->AddChild(card);

  parent->PerformLayout(300.0F, 200.0F);

  // Default card padding is 16px.
  const auto& child_bounds = child->GetBounds();
  EXPECT_FLOAT_EQ(child_bounds.x, 16.0F);
  EXPECT_FLOAT_EQ(child_bounds.y, 16.0F);
}

// ---------------------------------------------------------------------------
// ProgressIndicator
// ---------------------------------------------------------------------------

TEST(ProgressIndicatorTest, ValueIsClamped) {
  ProgressIndicator bar;
  bar.SetValue(2.0F);
  EXPECT_FLOAT_EQ(bar.GetValue(), 1.0F);
  bar.SetValue(-0.5F);
  EXPECT_FLOAT_EQ(bar.GetValue(), 0.0F);
  bar.SetValue(0.42F);
  EXPECT_FLOAT_EQ(bar.GetValue(), 0.42F);
}

TEST(ProgressIndicatorTest, OnMeasureProducesPositiveSize) {
  ProgressIndicator bar;
  const Size size = bar.OnMeasure(0.0F, 0, 0.0F, 0);
  EXPECT_GT(size.width, 0.0F);
  EXPECT_GT(size.height, 0.0F);
}

TEST(ProgressIndicatorTest, PaintEmitsTrackAndFillCommands) {
  ProgressIndicator bar;
  bar.SetValue(0.5F);
  bar.SetBounds(
      Rect{.x = 0.0F, .y = 0.0F, .width = 100.0F, .height = 8.0F});

  RenderContext context;
  bar.Paint(context);

  // Track plus fill rounded-rect commands.
  EXPECT_GE(context.GetCommandCount(), 2U);
}

TEST(ProgressIndicatorTest, ZeroValueDrawsOnlyTrack) {
  ProgressIndicator bar;
  bar.SetValue(0.0F);
  bar.SetBounds(
      Rect{.x = 0.0F, .y = 0.0F, .width = 100.0F, .height = 8.0F});

  RenderContext context;
  bar.Paint(context);

  EXPECT_EQ(context.GetCommandCount(), 1U);
}

// ---------------------------------------------------------------------------
// Switch
// ---------------------------------------------------------------------------

TEST(SwitchTest, DefaultsToUnchecked) {
  Switch sw;
  EXPECT_FALSE(sw.IsChecked());
}

TEST(SwitchTest, SetCheckedAndOnPointerDownToggle) {
  Switch sw;
  sw.SetChecked(true);
  EXPECT_TRUE(sw.IsChecked());

  bool observed = false;
  bool fired = false;
  sw.SetOnChanged([&](bool value) {
    fired = true;
    observed = value;
  });

  sw.SetBounds(
      Rect{.x = 0.0F, .y = 0.0F, .width = 60.0F, .height = 30.0F});
  EXPECT_TRUE(sw.OnPointerDown(Point{.x = 10.0F, .y = 10.0F}));

  // Was checked; a pointer down flips it to false and notifies.
  EXPECT_FALSE(sw.IsChecked());
  EXPECT_TRUE(fired);
  EXPECT_FALSE(observed);
}

TEST(SwitchTest, OnMeasureProducesPositiveSize) {
  Switch sw;
  const Size size = sw.OnMeasure(0.0F, 0, 0.0F, 0);
  EXPECT_GT(size.width, 0.0F);
  EXPECT_GT(size.height, 0.0F);
}

// ---------------------------------------------------------------------------
// Checkbox
// ---------------------------------------------------------------------------

TEST(CheckboxTest, DefaultsToUnchecked) {
  Checkbox box;
  EXPECT_FALSE(box.IsChecked());
}

TEST(CheckboxTest, OnPointerDownFlipsStateAndNotifies) {
  Checkbox box;
  bool observed = true;
  box.SetOnChanged([&](bool value) { observed = value; });

  box.SetBounds(
      Rect{.x = 0.0F, .y = 0.0F, .width = 22.0F, .height = 22.0F});
  EXPECT_TRUE(box.OnPointerDown(Point{.x = 5.0F, .y = 5.0F}));

  EXPECT_TRUE(box.IsChecked());
  EXPECT_TRUE(observed);

  EXPECT_TRUE(box.OnPointerDown(Point{.x = 5.0F, .y = 5.0F}));
  EXPECT_FALSE(box.IsChecked());
  EXPECT_FALSE(observed);
}

TEST(CheckboxTest, PaintEmitsCommandsWhenChecked) {
  Checkbox box;
  box.SetChecked(true);
  box.SetBounds(
      Rect{.x = 0.0F, .y = 0.0F, .width = 22.0F, .height = 22.0F});

  RenderContext context;
  box.Paint(context);

  // Box fill plus the centered check mark.
  EXPECT_GE(context.GetCommandCount(), 2U);
}

}  // namespace
}  // namespace neoflux
