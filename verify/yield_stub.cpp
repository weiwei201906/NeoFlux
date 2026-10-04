// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors

// Minimal stand-in for the EventLoop-backed Yield/Sleep awaitables, so that
// task.h can be compiled and its continuation logic exercised in isolation.
//
// The real implementations hand the handle to EventLoop::ScheduleYield, which
// resumes it on the NEXT frame. To model that one-frame deferral here, the
// stub stores the handle and the test drains it explicitly.
#include <coroutine>
#include <vector>

#include "neoflux/core/task.h"

namespace neoflux {

namespace {
std::vector<std::coroutine_handle<>> g_deferred;
}  // namespace

void YieldAwaitable::await_suspend(std::coroutine_handle<> h) {
  // Defer to the next "frame" instead of resuming inline, matching the real
  // EventLoop behaviour (one-frame yield).
  g_deferred.push_back(h);
}

void SleepAwaitable::await_suspend(std::coroutine_handle<> h) {
  g_deferred.push_back(h);
}

}  // namespace neoflux

// Test helper: resume everything deferred by Yield()/Sleep() (one frame).
extern "C" void DrainDeferredFrame() {
  auto pending = std::move(neoflux::g_deferred);
  neoflux::g_deferred.clear();
  for (auto h : pending) {
    if (h && !h.done()) h.resume();
  }
}

extern "C" std::size_t DeferredCount() { return neoflux::g_deferred.size(); }
