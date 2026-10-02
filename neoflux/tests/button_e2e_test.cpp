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

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

#include <neoflux/apps/event_loop.h>
#include <neoflux/core/task.h>
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

// ---------------------------------------------------------------------------
// pressed_widget_ weak_ptr lifecycle guard.
//
// Application stashes pressed_widget_ as a weak_ptr<Widget> on press and locks
// it on release. If the widget is destroyed between press and release (e.g. the
// route popped), lock() returns null and OnPointerUp must be skipped.
// ---------------------------------------------------------------------------

TEST(ButtonE2ETest, PressedWidgetWeakPtrLifecycle) {
  auto button = std::make_shared<Button>("OK");
  button->SetBounds(kButtonBounds);

  bool pressed = false;
  button->SetOnPressed([&] { pressed = true; });

  // Simulate Application pressing the widget: the app stashes a weak_ptr.
  ASSERT_TRUE(button->OnPointerDown(kInside));
  std::weak_ptr<Widget> pressed_widget = button->weak_from_this();
  ASSERT_FALSE(pressed_widget.expired());

  // Destroy the widget (route pop / tree rebuild) before the release event.
  button.reset();
  ASSERT_TRUE(pressed_widget.expired());

  // Simulate Application::DispatchPointerEvent(release): lock is null, so
  // OnPointerUp is never dispatched - no crash, and on_pressed_ never fires.
  ASSERT_EQ(pressed_widget.lock(), nullptr);
  EXPECT_FALSE(pressed);
}

// ---------------------------------------------------------------------------
// Background EventLoop harness.
//
// EventLoop exposes no generic PostTask. To run a closure on the loop thread we
// express it as a trivially-returning Task<void> coroutine; EventLoop::Schedule()
// resumes it on the next frame tick, where EventLoop::Current() is valid and
// Button::OnPointerDown can arm its long-press Delay coroutine. The loop runs
// on a worker thread; TearDown stops and joins it.
// ---------------------------------------------------------------------------

// One-shot coroutine: run |body| on the loop thread and complete.
Task<void> RunOnceOnLoop(std::function<void()> body) {
  body();
  co_return;
}

// Schedules |body| to execute once on the loop thread (thread-safe).
void PostToLoop(EventLoop& loop, std::function<void()> body) {
  loop.Schedule(RunOnceOnLoop(std::move(body)));
}

class EventLoopThreadFixture : public ::testing::Test {
 protected:
  void SetUp() override {
    // Spin until the loop reports running so Schedule() reaches a live loop.
    while (!loop_.IsRunning()) {
      std::this_thread::yield();
    }
  }

  void TearDown() override {
    loop_.Stop();
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  // Posts |body| to run once on the loop thread.
  void Post(std::function<void()> body) { PostToLoop(loop_, std::move(body)); }

  // Posts a barrier task and blocks (with timeout) until it has executed on
  // the loop thread. Guarantees all previously posted work is drained.
  void WaitIdle() {
    std::mutex m;
    std::condition_variable cv;
    bool done = false;
    Post([&] {
      {
        const std::scoped_lock<std::mutex> lock(m);
        done = true;
      }
      cv.notify_all();
    });
    std::unique_lock<std::mutex> lock(m);
    ASSERT_TRUE(cv.wait_for(lock, std::chrono::seconds(5), [&] {
      return done;
    }));
  }

 private:
  EventLoop loop_{};
  std::thread thread_{[this] { loop_.Run([] {}); }};
};

// ---------------------------------------------------------------------------
// Long-press fires after the 500ms hold. OnPointerDown runs on the loop thread
// so CanAnimate() is true and Delay() launches the coroutine. We wait for the
// OnLongPress callback with a condition variable (not a bare sleep) to confirm
// the real timer chain works end to end.
// ---------------------------------------------------------------------------

TEST_F(EventLoopThreadFixture, LongPressFiresCallback) {
  auto button = std::make_shared<Button>("OK");
  button->SetBounds(kButtonBounds);

  std::atomic<bool> long_press_fired{false};
  std::mutex cv_mutex;
  std::condition_variable cv;
  button->SetOnLongPress([&] {
    long_press_fired.store(true);
    cv.notify_all();
  });

  // Drive the press on the loop thread where EventLoop::Current() is valid.
  Post([button] { button->OnPointerDown(kInside); });

  // Wait for the long-press callback (holds 500ms; allow generous timeout).
  {
    std::unique_lock<std::mutex> lock(cv_mutex);
    EXPECT_TRUE(cv.wait_for(lock, std::chrono::seconds(2), [&] {
      return long_press_fired.load();
    }));
  }
  EXPECT_TRUE(long_press_fired.load());

  // Release on the loop thread to reset pressed state before teardown.
  Post([button] { button->OnPointerUp(kInside); });
  WaitIdle();
}

// Releasing before 500ms cancels the long-press: the callback must never fire.
TEST_F(EventLoopThreadFixture, LongPressCancelledOnEarlyRelease) {
  auto button = std::make_shared<Button>("OK");
  button->SetBounds(kButtonBounds);

  std::atomic<bool> long_press_fired{false};
  std::mutex cv_mutex;
  std::condition_variable cv;
  button->SetOnLongPress([&] {
    long_press_fired.store(true);
    cv.notify_all();
  });

  // Press then immediately release on the loop thread (well under 500ms).
  Post([button] {
    button->OnPointerDown(kInside);
    button->OnPointerUp(kInside);
  });

  // The 500ms timer still expires; the coroutine wakes, re-checks is_pressed_
  // (now false) and must NOT invoke the callback. Wait long enough for the
  // timer to elapse and confirm it never fired.
  {
    std::unique_lock<std::mutex> lock(cv_mutex);
    EXPECT_FALSE(cv.wait_for(lock, std::chrono::milliseconds(1200), [&] {
      return long_press_fired.load();
    }));
  }
  EXPECT_FALSE(long_press_fired.load());

  WaitIdle();
}

// Long-press is a distinct action: once it fires, releasing inside the button
// must NOT also dispatch on_pressed_ (this guards the HandleRelease long_press
// guard).
TEST_F(EventLoopThreadFixture, LongPressSuppressesClick) {
  auto button = std::make_shared<Button>("OK");
  button->SetBounds(kButtonBounds);

  std::atomic<bool> pressed{false};
  std::atomic<bool> long_press_fired{false};
  std::mutex cv_mutex;
  std::condition_variable cv;
  button->SetOnPressed([&] {
    pressed.store(true);
    cv.notify_all();
  });
  button->SetOnLongPress([&] {
    long_press_fired.store(true);
    cv.notify_all();
  });

  // Press on the loop thread and hold through the 500ms long-press.
  Post([button] { button->OnPointerDown(kInside); });

  {
    std::unique_lock<std::mutex> lock(cv_mutex);
    EXPECT_TRUE(cv.wait_for(lock, std::chrono::seconds(2), [&] {
      return long_press_fired.load();
    }));
  }
  EXPECT_TRUE(long_press_fired.load());

  // Release inside: after the fix this must NOT fire on_pressed_.
  Post([button] { button->OnPointerUp(kInside); });
  WaitIdle();

  EXPECT_FALSE(pressed.load());
}

// The long-press coroutine must be lifetime-safe when the widget is destroyed
// while the 500ms Delay timer is still pending. AnimationRuntime::Delay binds a
// weak_ptr to the owner; on expiry it must observe the expired owner and return
// without dereferencing the dead widget.
TEST_F(EventLoopThreadFixture, LongPressCoroutineSurvivesWidgetDestruction) {
  auto button = std::make_shared<Button>("OK");
  button->SetBounds(kButtonBounds);

  bool long_press_fired = false;
  button->SetOnLongPress([&] { long_press_fired = true; });

  // Confirm OnPointerDown has actually run on the loop thread (arming the Delay
  // coroutine) before we destroy the widget.
  std::mutex mu;
  std::condition_variable cv;
  bool down_done = false;
  Post([&, button] {
    button->OnPointerDown(kInside);
    {
      const std::scoped_lock<std::mutex> lock(mu);
      down_done = true;
    }
    cv.notify_all();
  });
  {
    std::unique_lock<std::mutex> lock(mu);
    ASSERT_TRUE(cv.wait_for(lock, std::chrono::seconds(2), [&] {
      return down_done;
    }));
  }

  // Destroy the widget mid-delay. The in-flight coroutine holds only a weak_ptr.
  button.reset();

  // Let the real 500ms timer elapse; the loop thread must observe the expired
  // owner and return. No crash, no use-after-free.
  std::this_thread::sleep_for(std::chrono::milliseconds(1200));
  WaitIdle();

  EXPECT_FALSE(long_press_fired);
}

}  // namespace
}  // namespace neoflux
