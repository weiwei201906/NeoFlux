// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors

// Verifies the EventLoop behaviour (P1-2, P1-3 fixes + idle throttling):
//   1a. --idle_fps=0: pure pacing honours target_fps (legacy full-rate mode).
//   1b. Default idle throttling: a loop with no render requests and no
//       pending coroutine/timer work drops to the idle heart-beat rate.
//   1c. Animation protection: a pending Sleep() coroutine (the tween/
//       animation driver) keeps the loop at full target_fps.
//   2. A burst of WakeUp() calls is coalesced into a single early frame,
//      instead of one frame per notification.
//   3. Stop() drains coroutine state without touching a freed frame.
#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>

#include <glog/logging.h>

#include "neoflux/apps/event_loop.h"
#include "neoflux/core/flags.h"

using namespace neoflux;

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  FLAGS_logtostderr = false;
  int failures = 0;

  // --- Case 1a: idle throttling disabled => pacing must track target FPS ---
  {
    FLAGS_idle_fps = 0;
    EventLoop loop;
    loop.SetTargetFps(60);
    std::atomic<int> frames{0};
    std::thread r([&] { loop.Run([&] { frames.fetch_add(1); }); });
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    loop.Stop();
    r.join();
    FLAGS_idle_fps = 15;  // Restore the default for the following cases.
    const int n = frames.load();
    std::printf("case 1a: %d frames in ~1s @60fps (idle_fps=0)\n", n);
    // Generous band: CI machines are noisy, but 60 +/- 15 catches both a
    // runaway loop and a loop that barely runs.
    if (n < 45 || n > 75) {
      std::printf("  [FAIL] pacing outside 60fps +/- 25%%\n");
      ++failures;
    } else {
      std::printf("  [PASS] full-rate pacing tracks target FPS\n");
    }
  }

  // --- Case 1b: idle loop drops to the idle heart-beat rate ---
  {
    FLAGS_idle_fps = 15;
    EventLoop loop;
    loop.SetTargetFps(60);
    std::atomic<int> frames{0};
    std::thread r([&] { loop.Run([&] { frames.fetch_add(1); }); });
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    loop.Stop();
    r.join();
    const int n = frames.load();
    std::printf("case 1b: %d frames in ~1s idle (heartbeat 15fps)\n", n);
    // ~3 full-rate frames before the throttle engages, then ~15 heart-beat
    // frames; wide band for scheduler noise on CI machines.
    if (n < 12 || n > 26) {
      std::printf("  [FAIL] idle heart-beat outside 15fps + startup band\n");
      ++failures;
    } else {
      std::printf("  [PASS] idle loop throttled to heart-beat\n");
    }
  }

  // --- Case 1c: pending Sleep() coroutine keeps the loop at full rate ---
  {
    FLAGS_idle_fps = 15;
    EventLoop loop;
    loop.SetTargetFps(60);
    std::atomic<int> frames{0};
    // A 50-step, 16 ms ticker: the same pattern the animation runtime uses
    // (TweenCoroutine). While its timer is pending the loop must NOT throttle.
    auto ticker = []() -> Task<void> {
      for (int i = 0; i < 50; ++i) {
        co_await Sleep(std::chrono::milliseconds(16));
      }
    };
    std::thread r([&] {
      loop.Schedule(ticker());
      loop.Run([&] { frames.fetch_add(1); });
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    loop.Stop();
    r.join();
    const int n = frames.load();
    std::printf("case 1c: %d frames in ~1s with active timer coroutine\n", n);
    if (n < 45 || n > 75) {
      std::printf("  [FAIL] animation did not keep the loop at full rate\n");
      ++failures;
    } else {
      std::printf("  [PASS] animation keeps the loop at full rate\n");
    }
  }

  // --- Case 2: a burst of WakeUp() must coalesce, not add N frames ---
  {
    EventLoop loop;
    loop.SetTargetFps(60);  // ~16.7ms per frame
    std::atomic<int> frames{0};
    std::atomic<bool> started{false};
    std::thread r([&] {
      started.store(true);
      loop.Run([&] { frames.fetch_add(1); });
    });
    while (!started.load()) std::this_thread::yield();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    frames.store(0);
    // 50 notifications back-to-back, all arriving inside one frame interval.
    for (int i = 0; i < 50; ++i) loop.WakeUp();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    const int n = frames.load();
    loop.Stop();
    r.join();
    std::printf("case 2: %d frames after a 50x WakeUp burst (coalesced)\n", n);
    // One early frame plus the pacing frames in 100ms. Without coalescing
    // the burst alone would force ~50 extra frames.
    if (n > 20) {
      std::printf("  [FAIL] wake-up burst was not coalesced\n");
      ++failures;
    } else {
      std::printf("  [PASS] wake-up burst coalesced into one early frame\n");
    }
  }

  // --- Case 3: Stop() with pending coroutine state must be safe ---
  {
    EventLoop loop;
    loop.SetTargetFps(240);
    std::thread r([&] { loop.Run([] {}); });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    loop.Stop();  // must drain timer_queue_/yield_handles_/active_tasks_
    r.join();
    std::printf("case 3: [PASS] Stop() drained coroutine state cleanly\n");
  }

  if (failures == 0) {
    std::printf("\nAll EventLoop checks passed.\n");
    return 0;
  }
  std::printf("\n%d EventLoop check(s) FAILED.\n", failures);
  return 1;
}
