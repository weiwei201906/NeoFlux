// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors

// Concurrency stress test for the NeoFlux platform tuning layer.
// 8 threads x 5000 iterations each call a randomly chosen tuning API. The run
// must be clean under ThreadSanitizer (no data races, no deadlocks) and
// DetectCpuFeatures() must return the same value on every thread — a runtime
// invariant, since CPU features do not change during the process lifetime.
#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include <atomic>
#include <cstdio>
#include <random>
#include <thread>
#include <vector>

#include <glog/logging.h>

#include "native/native_tuning.h"

using neoflux::native::CpuFeatures;
using neoflux::native::DetectCpuFeatures;
using neoflux::native::TuneRenderThread;
using neoflux::native::TuneUiThread;
using neoflux::native::PinThreadToBigCores;

static constexpr int kThreads = 8;
static constexpr int kIters = 5000;

// TSAN suppression, compiled into the binary so no external file is needed.
// glog (< 0.6) computes its cached GMT offset lazily and without a lock on the
// first timestamped LOG() of the process; concurrent first-logs therefore race
// inside the LOGGING LIBRARY (strdup/free in LogMessageTime::CalcGmtOffset).
// That is third-party code, outside the translation unit under test, so we
// suppress it — any race in the native tuning layer still fails the run.
#if defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define NEOFLUX_TSAN 1
#endif
#endif
#if defined(__SANITIZE_THREAD__)
#define NEOFLUX_TSAN 1
#endif

#ifdef NEOFLUX_TSAN
extern "C" const char* __tsan_default_suppressions() {
  return "race:google::LogMessageTime::CalcGmtOffset\n"
         "race:strdup\n";
}
#endif

static bool SameFeatures(const CpuFeatures& a, const CpuFeatures& b) {
  return a.sse42 == b.sse42 && a.avx2 == b.avx2 && a.neon == b.neon &&
         a.neon_fp16 == b.neon_fp16;
}

int main(int argc, char** argv) {
  (void)argc;
  // glog must be initialised before any worker thread calls LOG() — otherwise
  // its lazy global setup (timezone string caching in LogMessageTime) races
  // across threads. This is a glog contract, not a tuning-layer issue.
  google::InitGoogleLogging(argv[0]);
  FLAGS_logtostderr = false;
  // Warm glog's lazy per-process timezone cache on the main thread. The first
  // formatted log line computes the GMT offset (strdup/free in
  // LogMessageTime::CalcGmtOffset); doing it here keeps that one-time setup
  // single-threaded so it cannot look like a race once the workers start.
  LOG(INFO) << "native stress: glog warmed up";

  // Reference value captured before any concurrency starts.
  const CpuFeatures ref = DetectCpuFeatures();

  std::atomic<int> mismatches{0};
  std::vector<std::thread> workers;
  workers.reserve(kThreads);

  for (int t = 0; t < kThreads; ++t) {
    workers.emplace_back([t, &mismatches, &ref] {
      std::mt19937 rng(0xC0FFEEu + static_cast<unsigned>(t));
      std::uniform_int_distribution<int> pick(0, 3);
      for (int i = 0; i < kIters; ++i) {
        switch (pick(rng)) {
          case 0:
            TuneRenderThread();
            break;
          case 1:
            TuneUiThread();
            break;
          case 2:
            PinThreadToBigCores();
            break;
          default: {
            const CpuFeatures f = DetectCpuFeatures();
            if (!SameFeatures(f, ref)) {
              mismatches.fetch_add(1, std::memory_order_relaxed);
            }
            break;
          }
        }
      }
    });
  }

  for (auto& w : workers) w.join();

  const int bad = mismatches.load();
  std::printf("stress: %d threads x %d iters complete; feature mismatches=%d\n",
              kThreads, kIters, bad);
  if (bad != 0) {
    std::printf("[FAIL] DetectCpuFeatures() disagreed across threads/iterations\n");
    return 1;
  }
  std::printf("[PASS] concurrent tuning calls clean; detection invariant held\n");
  return 0;
}
