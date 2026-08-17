// =============================================================================
// NeoFlux - event_loop.cpp
//
// Implementation of EventLoop. CV-driven frame scheduling: the loop blocks
// on a condition variable when idle and is woken on demand via WakeUp().
// Also supports C++20 coroutine scheduling via Schedule() and Sleep() via
// a timer queue.
// =============================================================================

#include "neoflux/app/event_loop.h"

#include <chrono>
#include <cstdint>
#include <utility>

#include <glog/logging.h>

namespace neoflux {

// Thread-local pointer to the active event loop. Set at the start of Run()
// and cleared at the end. Used by Sleep() to resolve the loop without
// passing it explicitly.
thread_local EventLoop* EventLoop::current_loop_ = nullptr;

EventLoop::EventLoop()
    : running_(false), should_stop_(false), frame_count_(0), target_fps_(60) {}

EventLoop::~EventLoop() { Stop(); }

EventLoop* EventLoop::Current() noexcept { return current_loop_; }

void EventLoop::Run(FrameCallback frame_callback) {
  if (running_.exchange(true)) {
    LOG(WARNING) << "EventLoop::Run called while already running";
    return;
  }

  // Set thread-local current pointer so Sleep() can find this loop.
  current_loop_ = this;

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

  current_loop_ = nullptr;
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
  auto shared_task = std::make_shared<Task<void>>(std::move(task));
  void* key = shared_task->Handle().address();
  {
    std::lock_guard<std::mutex> lock(coroutine_mutex_);
    active_tasks_.emplace(key, shared_task);
    pending_coroutines_.push_back(std::move(shared_task));
  }
  WakeUp();
}

void EventLoop::ScheduleYield(std::coroutine_handle<> continuation) {
  {
    std::lock_guard<std::mutex> lock(coroutine_mutex_);
    yield_handles_.push_back(continuation);
  }
  WakeUp();
}

void EventLoop::ScheduleSleep(std::chrono::steady_clock::duration duration,
                              std::coroutine_handle<> continuation) {
  const auto wake_time = std::chrono::steady_clock::now() + duration;
  {
    std::lock_guard<std::mutex> lock(coroutine_mutex_);
    timer_queue_.emplace(wake_time, continuation);
  }
  WakeUp();
}

void YieldAwaitable::await_suspend(std::coroutine_handle<> h) {
  EventLoop* loop = EventLoop::Current();
  if (loop != nullptr) {
    loop->ScheduleYield(h);
  } else {
    // No active loop: resume immediately.
    h.resume();
  }
}

void SleepAwaitable::await_suspend(std::coroutine_handle<> h) {
  EventLoop* loop = EventLoop::Current();
  if (loop != nullptr) {
    loop->ScheduleSleep(duration, h);
  } else {
    // No active loop: resume immediately (cannot schedule a timer).
    h.resume();
  }
}

void EventLoop::RunReadyCoroutines() {
  // Phase 1: promote yield-pending handles back to pending so they resume
  // this frame. These are coroutines that did co_await Yield().
  {
    std::lock_guard<std::mutex> lock(coroutine_mutex_);
    for (const auto& handle : yield_handles_) {
      auto it = active_tasks_.find(handle.address());
      if (it != active_tasks_.end()) {
        pending_coroutines_.push_back(it->second);
      }
    }
    yield_handles_.clear();
  }

  // Phase 2: move pending coroutines out and resume them.
  std::vector<std::shared_ptr<Task<void>>> ready;
  {
    std::lock_guard<std::mutex> lock(coroutine_mutex_);
    ready.swap(pending_coroutines_);
  }
  for (const auto& task : ready) {
    if (task != nullptr && !task->Done()) {
      task->Resume();
    }
  }

  // Phase 3: erase completed tasks from active_tasks_. The shared_ptr in
  // |ready| keeps the frame alive until this function returns; after erase
  // and |ready| going out of scope, the refcount drops and the frame is
  // destroyed if no timer/yield handle still references it.
  {
    std::lock_guard<std::mutex> lock(coroutine_mutex_);
    for (const auto& task : ready) {
      if (task != nullptr && task->Done()) {
        active_tasks_.erase(task->Handle().address());
      }
    }
  }

  // Phase 4: resume expired sleep timers. Look up the owning Task from
  // active_tasks_ so we never resume a dangling handle.
  const auto now = std::chrono::steady_clock::now();
  std::vector<std::coroutine_handle<>> expired;
  {
    std::lock_guard<std::mutex> lock(coroutine_mutex_);
    auto it = timer_queue_.begin();
    while (it != timer_queue_.end() && it->first <= now) {
      expired.push_back(it->second);
      it = timer_queue_.erase(it);
    }
  }
  for (const auto& handle : expired) {
    std::shared_ptr<Task<void>> task;
    {
      std::lock_guard<std::mutex> lock(coroutine_mutex_);
      auto it = active_tasks_.find(handle.address());
      if (it != active_tasks_.end()) {
        task = it->second;
      }
    }
    if (task != nullptr && !task->Done()) {
      task->Resume();
      if (task->Done()) {
        std::lock_guard<std::mutex> lock(coroutine_mutex_);
        active_tasks_.erase(handle.address());
      }
    }
  }
}

}  // namespace neoflux
