// =============================================================================
// NeoFlux - event_loop.h
//
// Application-layer event loop. All method implementations are in
// event_loop.cpp.
// =============================================================================

#ifndef NEOFLUX_APP_EVENT_LOOP_H_
#define NEOFLUX_APP_EVENT_LOOP_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <coroutine>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "neoflux/core/noncopyable.h"
#include "neoflux/core/task.h"

namespace neoflux {

// Application-layer event loop.
//
// Runs the frame pipeline driven by a condition variable. The loop blocks
// on a CV when no work is pending, and is woken immediately when WakeUp()
// is called (e.g. when a widget is marked dirty or an input event arrives).
// A maximum wait timeout enforces the target frame rate.
//
// Also supports C++20 coroutine scheduling: Schedule() accepts a Task<T>
// and resumes it on the event-loop thread, enabling async-style code.
class EventLoop : public NonCopyable {
 public:
  // Frame callback invoked once per frame.
  using FrameCallback = std::function<void()>;

  EventLoop();
  ~EventLoop();

  // Runs the event loop until Stop() is called or the window closes.
  void Run(FrameCallback frame_callback);

  // Requests the event loop to stop after the current frame.
  void Stop() noexcept;

  // Wakes the event loop immediately, causing a frame to be processed
  // without waiting for the next frame-rate timeout. Thread-safe.
  void WakeUp() noexcept;

  // Returns true if the loop is currently running.
  [[nodiscard]] bool IsRunning() const noexcept;

  // Sets the target frames per second (default: 60).
  void SetTargetFps(int fps) noexcept;

  // Returns the target FPS.
  [[nodiscard]] int GetTargetFps() const noexcept;

  // Returns the number of frames processed since Run() started.
  [[nodiscard]] uint64_t GetFrameCount() const noexcept;

  // Schedules a coroutine task to be resumed on the event-loop thread.
  // The task is moved into the loop and resumed on the next frame tick.
  // Thread-safe.
  void Schedule(Task<void> task);

  // Schedules a coroutine handle to resume on the next frame tick.
  // Used by YieldAwaitable. Thread-safe.
  void ScheduleYield(std::coroutine_handle<> continuation);

  // Schedules a coroutine handle to resume after the given duration.
  // Used by SleepAwaitable. Thread-safe.
  void ScheduleSleep(std::chrono::steady_clock::duration duration,
                     std::coroutine_handle<> continuation);

  // Returns the event loop running on the current thread, or nullptr.
  // Set at the start of Run(); used by Sleep() to find the active loop.
  [[nodiscard]] static EventLoop* Current() noexcept;

 private:
  // Resumes all ready coroutines and expired sleep timers. Called once per
  // frame.
  void RunReadyCoroutines();

  std::atomic<bool> running_{false};
  std::atomic<bool> should_stop_{false};
  std::atomic<uint64_t> frame_count_{0};
  int target_fps_;

  // CV used to block the loop when idle and wake it on demand.
  std::condition_variable frame_cv_{};
  std::mutex frame_mutex_{};

  // Pending coroutines to resume. Guarded by coroutine_mutex_.
  // Shared ownership: active_tasks_ also holds a reference for the lifetime
  // of the coroutine, preventing premature frame destruction while a timer
  // or yield-pending handle still references it.
  std::mutex coroutine_mutex_{};
  std::vector<std::shared_ptr<Task<void>>> pending_coroutines_{};

  // All active (not-yet-completed) tasks, keyed by coroutine handle address.
  // This map is the authoritative owner: when a task completes, it is erased
  // here and the shared_ptr refcount drops, eventually destroying the frame.
  // Guarded by coroutine_mutex_.
  std::unordered_map<void*, std::shared_ptr<Task<void>>> active_tasks_{};

  // Handles that requested a one-frame yield (co_await Yield()).
  // Resumed at the start of the next frame. Guarded by coroutine_mutex_.
  std::vector<std::coroutine_handle<>> yield_handles_{};

  // Timer queue: wake-up time -> coroutine handle to resume.
  // Guarded by coroutine_mutex_.
  std::multimap<std::chrono::steady_clock::time_point,
                std::coroutine_handle<>>
      timer_queue_{};

  // Thread-local pointer to the running event loop, set during Run().
  static thread_local EventLoop* current_loop_;
};

}  // namespace neoflux

#endif  // NEOFLUX_APP_EVENT_LOOP_H_
