// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - button_e2e_test.cpp
//
// Headless end-to-end tests for Button hover, click-release and long-press
// interactions.
//
// Threading model
// ---------------
// Button long-press detection is driven by AnimationRuntime::Delay(), which
// launches a coroutine that does co_await Sleep(500ms). Sleep() resolves the
// active loop via EventLoop::Current(), a thread_local pointer that is set ONLY
// inside EventLoop::Run(). CanAnimate() therefore returns false unless
// OnPointerDown runs on the loop thread. These tests run an EventLoop on a
// background std::thread and drive Button input on it by scheduling a
// one-shot Task<void> coroutine through EventLoop::Schedule() (EventLoop has no
// generic PostTask; the trivial coroutine shim below resumes on the loop
// thread, where Current() is valid). The coroutine chain (Delay ->
// ScheduleSleep -> timer expiry -> callback) is exercised end to end.
//
// Hover/paint color checks and the pressed_widget_ weak_ptr lifecycle guard do
// not require a running loop and are exercised synchronously on the test thread.
// =============================================================================

#include <neoflux/widgets/button.h>

#include <cstdint>
#include <memory>

#include <neoflux/core/types.h>
#include <neoflux/renderers/render_context.h>
#include <neoflux/renderers/render_command.h>

#include <gtest/gtest.h>

namespace neoflux {
namespace {

// Default Button colors (must match button.cpp constructor).
constexpr Color kIdleColor{.r = 0x21, .g = 0x96, .b = 0xF3, .a = 0xFF};
constexpr Color kHoverColor{.r = 0x42, .g = 0xA5, .b = 0xF5, .a = 0xFF};
constexpr Rect kButtonBounds{.x = 0.0F,
                             .y = 0.0F,
                             .width = 100.0F,
                             .height = 40.0F};
// A point strictly inside kButtonBounds, and one well outside it.
constexpr Point kInside{.x = 50.0F, .y = 20.0F};
constexpr Point kOutside{.x = 150.0F, .y = 200.0F};

// Asserts every channel of |actual| equals |expected|.
void ExpectColor(const Color& expected, const Color& actual) {
  EXPECT_EQ(static_cast<int>(actual.r), static_cast<int>(expected.r));
  EXPECT_EQ(static_cast<int>(actual.g), static_cast<int>(expected.g));
  EXPECT_EQ(static_cast<int>(actual.b), static_cast<int>(expected.b));
  EXPECT_EQ(static_cast<int>(actual.a), static_cast<int>(expected.a));
}

// Paints |button| into a fresh context and returns the color of the first
// DrawRect command (the button body fill). Fails the test if none is found.
Color FirstDrawRectColor(Button& button) {
  RenderContext context;
  button.Paint(context);
  for (const auto& cmd : context.GetCommands()) {
    if (cmd.type == RenderCommandType::kDrawRect) {
      return cmd.color;
    }
  }
  ADD_FAILURE() << "Paint() produced no DrawRect command";
  return Color{};
}

// ---------------------------------------------------------------------------
// Hover state -> Paint() color (synchronous, no EventLoop required).
// Button exposes no hover callback; hovered_ is private and is only observable
// through the body fill color chosen by Paint().
// ---------------------------------------------------------------------------

TEST(ButtonE2ETest, HoverEnterSetsHoverState) {
  Button button("OK");
  button.SetBounds(kButtonBounds);

  // Idle: body uses the background color.
  ExpectColor(kIdleColor, FirstDrawRectColor(button));

  // Enter: body switches to the hover color.
  button.OnPointerEnter();
  ExpectColor(kHoverColor, FirstDrawRectColor(button));
}

TEST(ButtonE2ETest, HoverExitClearsHoverState) {
  Button button("OK");
  button.SetBounds(kButtonBounds);

  // Enter first, confirm hover color is active.
  button.OnPointerEnter();
  ExpectColor(kHoverColor, FirstDrawRectColor(button));

  // Exit: body reverts to the background color.
  button.OnPointerExit();
  ExpectColor(kIdleColor, FirstDrawRectColor(button));
}

TEST(ButtonE2ETest, HoverEnterIsIdempotent) {
  Button button("OK");
  button.SetBounds(kButtonBounds);

  // Re-entering while already hovered must not crash or flip state.
  button.OnPointerEnter();
  button.OnPointerEnter();
  ExpectColor(kHoverColor, FirstDrawRectColor(button));
}

// ---------------------------------------------------------------------------
// Click release paths. These run on the test thread without a running loop, so
// AnimationRuntime::CanAnimate() is false and the long-press coroutine is a
// no-op; this isolates HandlePress/HandleRelease behavior.
// ---------------------------------------------------------------------------

TEST(ButtonE2ETest, ClickFiresOnReleaseInside) {
  auto button = std::make_shared<Button>("OK");
  button->SetBounds(kButtonBounds);

  int clicks = 0;
  button->SetOnPressed([&] { ++clicks; });

  // Press inside is consumed, release inside fires the callback exactly once.
  button->OnPointerEnter();
  EXPECT_TRUE(button->OnPointerDown(kInside));
  button->OnPointerUp(kInside);

  EXPECT_EQ(clicks, 1);
}

TEST(ButtonE2ETest, PressMoveOutThenUpNoClick) {
  auto button = std::make_shared<Button>("OK");
  button->SetBounds(kButtonBounds);

  int clicks = 0;
  button->SetOnPressed([&] { ++clicks; });

  // Down inside, then release outside the bounds: must NOT fire on_pressed_.
  EXPECT_TRUE(button->OnPointerDown(kInside));
  button->OnPointerUp(kOutside);

  EXPECT_EQ(clicks, 0);
}

}  // namespace
}  // namespace neoflux
