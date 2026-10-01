// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - animation.cpp
//
// Implementation of the reusable coroutine-handshake runtime. See
// animation.h for the design rationale.
// =============================================================================

#include "neoflux/core/animation.h"

#include <utility>

#include "neoflux/apps/event_loop.h"
#include "neoflux/widgets/widget.h"

namespace neoflux {

namespace {

// ~16ms per frame (~60fps) tick used by tween coroutines.
constexpr auto kFrameTick = std::chrono::milliseconds(16);

// Sleep()-ticked tween coroutine. Re-arms every frame, locks the owning
// widget, interpolates, and exits naturally when the widget is gone or the
// tween completes. This is NOT an OnFrame accumulator: each wake is a fresh
// co_await Sleep suspension driven by the EventLoop timer queue.
Task<void> TweenCoroutine(std::weak_ptr<Widget> weak, float from, float to,
                          std::chrono::milliseconds duration,
                          std::function<void(float)> on_step,
                          std::function<void()> on_done) {
  const int total_steps =
      duration.count() <= 0
          ? 1
          : static_cast<int>((duration.count() + kFrameTick.count() - 1) /
                             kFrameTick.count());
  for (int step = 1; step <= total_steps; ++step) {
    co_await Sleep(kFrameTick);
    auto self = weak.lock();
    if (!self) {
      co_return;  // Widget destroyed mid-animation.
    }
    const float t = static_cast<float>(step) / static_cast<float>(total_steps);
    on_step(from + ((to - from) * t));
  }
  if (on_done != nullptr) {
    on_done();
  }
}

// Delay coroutine: suspend for |delay|, then fire |callback| if the owning
// widget still exists. The callback itself is expected to re-check the widget
// state before acting (condition-lock pattern).
Task<void> DelayCoroutine(std::weak_ptr<Widget> weak,
                          std::chrono::milliseconds delay,
                          std::function<void()> callback) {
  co_await Sleep(delay);
  auto self = weak.lock();
  if (!self) {
    co_return;  // Widget destroyed during the delay.
  }
  if (callback != nullptr) {
    callback();
  }
}

}  // namespace

struct AnimationRuntime::Impl {
  std::weak_ptr<Widget> owner;
};

AnimationRuntime::AnimationRuntime() : impl_(std::make_unique<Impl>()) {}

AnimationRuntime::~AnimationRuntime() = default;

AnimationRuntime::AnimationRuntime(AnimationRuntime&& other) noexcept = default;

AnimationRuntime& AnimationRuntime::operator=(
    AnimationRuntime&& other) noexcept = default;

void AnimationRuntime::Bind(std::weak_ptr<Widget> owner) {
  impl_->owner = std::move(owner);
}

bool AnimationRuntime::CanAnimate() const noexcept {
  return !impl_->owner.expired() && EventLoop::Current() != nullptr;
}

void AnimationRuntime::Tween(float from, float to,
                            std::chrono::milliseconds duration,
                            std::function<void(float)> on_step,
                            std::function<void()> on_done) {
  if (!CanAnimate()) {
    return;
  }
  EventLoop::Current()->Schedule(TweenCoroutine(
      impl_->owner, from, to, duration, std::move(on_step), std::move(on_done)));
}

void AnimationRuntime::Delay(std::chrono::milliseconds delay,
                            std::function<void()> callback) {
  if (!CanAnimate()) {
    return;
  }
  EventLoop::Current()->Schedule(
      DelayCoroutine(impl_->owner, delay, std::move(callback)));
}

}  // namespace neoflux
