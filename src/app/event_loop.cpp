// =============================================================================
// NeoFlux - event_loop.cpp
//
// Implementation of EventLoop. Methods moved from header.
// =============================================================================

#include "neoflux/app/event_loop.h"

#include <chrono>
#include <thread>

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

  LOG(INFO) << "EventLoop started at " << target_fps_ << " fps";

  while (!should_stop_.load()) {
    const auto frame_start = std::chrono::steady_clock::now();

    if (frame_callback) {
      frame_callback();
    }

    frame_count_.fetch_add(1);
    ThrottleFrame(frame_start);
  }

  running_.store(false);
  LOG(INFO) << "EventLoop stopped after " << frame_count_.load() << " frames";
}

void EventLoop::Stop() noexcept { should_stop_.store(true); }

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

void EventLoop::ThrottleFrame(
    const std::chrono::steady_clock::time_point& frame_start) {
  const auto frame_duration = std::chrono::microseconds(
      static_cast<int64_t>(1'000'000.0 / static_cast<double>(target_fps_)));
  const auto elapsed = std::chrono::steady_clock::now() - frame_start;

  if (elapsed < frame_duration) {
    std::this_thread::sleep_for(frame_duration - elapsed);
  }
}

}  // namespace neoflux
