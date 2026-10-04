// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors

// Death / degradation test for the NeoFlux platform tuning layer.
// The contract says every entry point is best-effort: it must never throw,
// never abort and never crash, even under re-entry and after the affinity mask
// has been narrowed by the caller. This test hammers those paths and exits 0
// only if the process is still alive; any failure exits non-zero.
#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE  // cpu_set_t / sched_setaffinity
#endif

#include <cstdio>
#include <thread>

#include <sched.h>

#include "native/native_tuning.h"

using neoflux::native::DetectCpuFeatures;
using neoflux::native::TuneRenderThread;
using neoflux::native::TuneUiThread;
using neoflux::native::PinThreadToBigCores;

// Cycles through all four APIs; used to model arbitrary re-entry.
static void CallAll() {
  TuneRenderThread();
  TuneUiThread();
  PinThreadToBigCores();
  (void)DetectCpuFeatures();
}

int main() {
  int failures = 0;

  // --- Case A: 1000 consecutive re-entries, single thread ---
  for (int i = 0; i < 1000; ++i) {
    CallAll();
  }
  // Reaching this line proves no path aborted or crashed.
  std::printf("case A: 1000 single-thread re-entries of all four APIs "
              "completed\n");
  std::printf("  [PASS] no abort/crash under repeated re-entry\n");

  // --- Case B: call from main, then immediately from another thread ---
  {
    TuneRenderThread();
    TuneUiThread();
    PinThreadToBigCores();
    (void)DetectCpuFeatures();

    std::thread t([] {
      CallAll();
      CallAll();
    });
    t.join();
    std::printf("case B: main thread + joined worker both called all APIs\n");
    std::printf("  [PASS] cross-thread call and join survived\n");
  }

  // --- Case C: pin while already bound to a single core, then re-enter ---
  {
    cpu_set_t one;
    CPU_ZERO(&one);
    CPU_SET(0, &one);
    const int rc = sched_setaffinity(0, sizeof(one), &one);
    std::printf("case C: sched_setaffinity(single core) rc=%d\n", rc);
    // PinThreadToBigCores() reads cpufreq topology and may try to widen the
    // set again; on a machine with no cpufreq data it is a silent no-op. Then
    // re-enter everything on the (possibly narrowed) affinity.
    PinThreadToBigCores();
    for (int i = 0; i < 50; ++i) {
      CallAll();
    }
    std::printf("  [PASS] pin+re-entry from narrowed affinity did not crash\n");
  }

  // --- Case D: normal exit ---
  std::printf("case D: exiting cleanly with exit(0)\n");
  if (failures != 0) {
    std::printf("RESULT: %d failure(s)\n", failures);
    return 1;
  }
  std::printf("RESULT: death/degradation paths all survived\n");
  std::exit(0);
}
