// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// End-to-end check that every centralized flag is registered and parses,
// including the idle-pacing and native-tuning groups. Run from verify/README.
#include <cstdio>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include "neoflux/core/flags.h"
using namespace neoflux;

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  std::printf("target_fps=%d idle_fps=%d verbose=%d queue=%llu drop_log=%d\n",
              FLAGS_target_fps, FLAGS_idle_fps, (int)FLAGS_verbose_logging,
              (unsigned long long)FLAGS_render_queue_capacity,
              FLAGS_render_queue_drop_log_max);
  std::printf("native: tuning=%d rt_prio=%d nice=%d bigcore_permille=%d "
              "mmcss=%s timer_ms=%d\n",
              (int)FLAGS_native_tuning, FLAGS_native_render_rt_priority,
              FLAGS_native_thread_nice, FLAGS_native_bigcore_threshold_permille,
              FLAGS_native_mmcss_profile.c_str(), FLAGS_native_timer_period_ms);
  std::printf("[PASS] all centralized flags resolved\n");
  return 0;
}
