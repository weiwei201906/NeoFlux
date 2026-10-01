// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - animation.h
//
// Reusable coroutine-handshake runtime for state-machine-driven (dynamic)
// widgets. All method implementations are in animation.cpp.
//
// Dynamic widgets (Button, Switch, Checkbox, ProgressIndicator, Draggable,
// ScrollView) own an AnimationRuntime in their Pimpl Impl. A state transition
// either flips the widget's WidgetState or launches a short Sleep()-ticked
// coroutine through this runtime. Lifetime is guarded by a weak_ptr to the
// owning widget: if the widget is destroyed mid-animation, the in-flight
// coroutine observes the empty lock on its next wakeup and exits naturally -
// no explicit cancellation token is required.
//
// In headless / unit-test contexts there is no running EventLoop (and test
// widgets are often stack-allocated), so CanAnimate() is false and every
// animation call is a no-op. Callers then apply the final (target) visual
// state synchronously, which keeps the synchronous gtest expectations intact.
// =============================================================================

#ifndef NEOFLUX_CORE_ANIMATION_H_
#define NEOFLUX_CORE_ANIMATION_H_

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

#include "neoflux/core/task.h"

namespace neoflux {

class Widget;

// Reusable coroutine-handshake helper for state-machine-driven widgets.
//
// Pimpl'd: the only member is a std::unique_ptr<Impl>, so this header stays
// light and the weak_ptr/loop wiring lives in animation.cpp.
class AnimationRuntime {
 public:
  AnimationRuntime();
  ~AnimationRuntime();

  AnimationRuntime(AnimationRuntime&& other) noexcept;
  AnimationRuntime& operator=(AnimationRuntime&& other) noexcept;

  AnimationRuntime(const AnimationRuntime&) = delete;
  AnimationRuntime& operator=(const AnimationRuntime&) = delete;

  // Bind this runtime to the owning widget. Call on the event-loop thread at
  // the first animation trigger (e.g. OnPointerDown), where the widget is
  // shared-owned in production. Idempotent.
  void Bind(std::weak_ptr<Widget> owner);

  // True when an EventLoop is running on this thread AND the owner widget is
  // shared-owned. When false, callers must apply the final visual state
  // synchronously (headless / stack-allocated test widgets).
  [[nodiscard]] bool CanAnimate() const noexcept;

  // Tweens a float from |from| to |to| over |duration|, waking ~every 16ms.
  // |on_step| receives the eased (linear) value each frame and should repaint
  // the widget (typically via MarkNeedsBuild()); |on_done| fires once at the
  // end. No-op when CanAnimate() is false. The coroutine exits naturally if
  // the owning widget is destroyed mid-tween.
  void Tween(float from, float to, std::chrono::milliseconds duration,
             std::function<void(float)> on_step,
             std::function<void()> on_done = nullptr);

  // Fires |callback| after |delay|, guarded by the owner's lifetime. Used for
  // long-press detection: the callback re-checks the widget state before
  // acting (the "state machine as condition lock" pattern). No-op when
  // CanAnimate() is false.
  void Delay(std::chrono::milliseconds delay, std::function<void()> callback);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace neoflux

#endif  // NEOFLUX_CORE_ANIMATION_H_
