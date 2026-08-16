// =============================================================================
// NeoFlux - event_loop.h
//
// Application-layer event loop. All method implementations are in
// event_loop.cpp.
// =============================================================================

#ifndef NEOFLUX_APP_EVENT_LOOP_H_
#define NEOFLUX_APP_EVENT_LOOP_H_

#include <atomic>
#include <cstdint>
#include <functional>

#include "neoflux/core/noncopyable.h"

namespace neoflux {

// Application-layer event loop.
//
// Runs the frame pipeline at a target frame rate.
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

  // Returns true if the loop is currently running.
  [[nodiscard]] bool IsRunning() const noexcept;

  // Sets the target frames per second (default: 60).
  void SetTargetFps(int fps) noexcept;

  // Returns the target FPS.
  [[nodiscard]] int GetTargetFps() const noexcept;

  // Returns the number of frames processed since Run() started.
  [[nodiscard]] uint64_t GetFrameCount() const noexcept;

 private:
  // Sleeps for the remaining time in the current frame to maintain target FPS.
  void ThrottleFrame(
      const std::chrono::steady_clock::time_point& frame_start);

  std::atomic<bool> running_;
  std::atomic<bool> should_stop_;
  std::atomic<uint64_t> frame_count_;
  int target_fps_;
};

}  // namespace neoflux

#endif  // NEOFLUX_APP_EVENT_LOOP_H_
