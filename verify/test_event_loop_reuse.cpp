// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors

// Stress the P1-2 scenario hard: many sleeper coroutines, Stop() mid-flight,
// and re-Run() the SAME EventLoop afterwards. Re-Run is the case where stale
// timer_queue_ entries could actually be resumed (they were never cleared).
#include <chrono>
#include <cstdio>
#include <thread>
#include <glog/logging.h>
#include "neoflux/apps/event_loop.h"
#include "neoflux/core/task.h"
using namespace neoflux;

static std::atomic<int> g_resumed{0};
static Task<void> ShortSleeper() {
  co_await Sleep(std::chrono::milliseconds(30));
  g_resumed.fetch_add(1);
}

int main(int argc,char**argv){
  google::InitGoogleLogging(argv[0]); FLAGS_logtostderr=false;
  EventLoop loop; loop.SetTargetFps(1000);
  // Run #1: schedule sleepers, stop while their timers are still pending.
  {
    std::thread r([&]{ loop.Run([]{ }); });
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    for(int i=0;i<200;++i) loop.Schedule(ShortSleeper());
    loop.Stop();          // timers (30ms) have NOT fired yet
    r.join();
  }
  std::printf("after Run#1: resumed=%d (expect 0)\n", g_resumed.load());
  // Run #2: stale timer_queue_ entries from Run#1, if any, would fire now
  // and resume handles whose frames were already destroyed.
  {
    std::thread r([&]{ loop.Run([]{ }); });
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    loop.Stop(); r.join();
  }
  std::printf("after Run#2: resumed=%d\n", g_resumed.load());
}
