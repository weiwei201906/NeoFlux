// =============================================================================
// NeoFlux - event_loop.cpp
//
// Implementation of EventLoop. CV-driven frame scheduling: the loop blocks
// on a condition variable when idle and is woken on demand via WakeUp().
// Also supports C++20 coroutine scheduling via Schedule().
// =============================================================================

#include "neoflux/app/event_loop.h"

#include <chrono>
#include <cstdint>
#include <utility>

#include <glog/logging.h>

namespace neoflux {

EventLoop::EventLoop()
    : running_(false), should_stop_(false), frame_count_(0), target_fps_(60) {}

EventLoop::~EventLoop() { Stop(); }

void EventLoop::Run(FrameCallback frame_callback) {
  if (running_.exchange(true)) {
    LOG(WARNING) << "EventLoop::Run called while already running";
    return;
  }

  should_stop_.store(false);
  frame_count_.store(0);

  LOG(INFO) << "EventLoop started at " << target_fps_ << " fps (CV-driven)";

  const auto frame_duration = std::chrono::microseconds(
      static_cast<int64_t>(1'000'000.0 / static_cast<double>(target_fps_)));

  while (!should_stop_.load()) {
    if (frame_callback) {
      frame_callback();
    }
    RunReadyCoroutines();
    frame_count_.fetch_add(1);

    // Block until woken by WakeUp() or until the frame interval elapses.
    // This avoids busy-waiting when no work is pending.
    std::unique_lock<std::mutex> lock(frame_mutex_);
    frame_cv_.wait_for(lock, frame_duration,
                       [this] { return should_stop_.load(); });
  }

  running_.store(false);
  LOG(INFO) << "EventLoop stopped after " << frame_count_.load() << " frames";
}

void EventLoop::Stop() noexcept {
  should_stop_.store(true);
  frame_cv_.notify_one();
}

void EventLoop::WakeUp() noexcept { frame_cv_.notify_one(); }

bool EventLoop::IsRunning() const noexcept { return running_.load(); }

void EventLoop::SetTargetFps(int fps) noexcept {
  if (fps <= 0) {
    LOG(WARNING) << "Invalid target FPS: " << fps << ", using 60";
    target_fps_ = 60;
    return;
  }
  target_fps_ = fps;
}

int EventLoop::GetTargetFps() const noexcept { return target_fps_; }

uint64_t EventLoop::GetFrameCount() const noexcept {
  return frame_count_.load();
}

void EventLoop::Schedule(Task<void> task) {
  {
    std::lock_guard<std::mutex> lock(coroutine_mutex_);
    pending_coroutines_.push_back(std::move(task));
  }
  WakeUp();
}

void EventLoop::RunReadyCoroutines() {
  std::vector<Task<void>> ready;
  {
    std::lock_guard<std::mutex> lock(coroutine_mutex_);
    ready.swap(pending_coroutines_);
  }
  if (ready.empty()) {
    return;
  }

  for (auto& task : ready) {
    if (!task.Done()) {
      task.Resume();
    }
  }

  // Re-queue tasks that are not yet done (e.g. waiting on YieldAwaitable).
  for (auto& task : ready) {
    if (!task.Done()) {
      std::lock_guard<std::mutex> lock(coroutine_mutex_);
      pending_coroutines_.push_back(std::move(task));
    }
  }
}

}  // namespace neoflux
