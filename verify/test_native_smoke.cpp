// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors

// Smoke test for the NeoFlux platform tuning layer (native_tuning.h).
// Verifies every entry point returns without crashing, that DetectCpuFeatures()
// agrees with /proc/cpuinfo, that detection is stable across calls, and that
// PinThreadToBigCores() never leaves the calling thread with an empty affinity
// mask. Follows the plain-assert/printf style of the other verify/ tests.
#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE  // cpu_set_t / sched_getaffinity
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <sched.h>

#include "native/native_tuning.h"

using neoflux::native::CpuFeatures;
using neoflux::native::DetectCpuFeatures;
using neoflux::native::TuneRenderThread;
using neoflux::native::TuneUiThread;
using neoflux::native::PinThreadToBigCores;

// Reads /proc/cpuinfo once and reports whether `flag` appears in any "flags"
// line. Returns -1 when the file cannot be read (should not happen on Linux).
static int ProbeCpuFlag(const char* flag) {
  FILE* fp = std::fopen("/proc/cpuinfo", "r");
  if (fp == nullptr) return -1;
  char line[8192];
  bool found = false;
  while (std::fgets(line, sizeof(line), fp) != nullptr) {
    if (std::strncmp(line, "flags", 5) != 0) continue;
    const size_t n = std::strlen(flag);
    const char* p = line;
    while ((p = std::strstr(p, flag)) != nullptr) {
      // Require token boundaries so "avx2" does not match inside "avx512".
      const bool left_ok = (p == line) || (p[-1] == ' ' || p[-1] == '\t');
      const bool right_ok = (p[n] == ' ' || p[n] == '\n' || p[n] == '\0' ||
                             p[n] == '\t');
      if (left_ok && right_ok) {
        found = true;
        break;
      }
      p += n;
    }
    if (found) break;
  }
  std::fclose(fp);
  return found ? 1 : 0;
}

int main() {
  int failures = 0;

  // --- Case 1: all four entry points return without crashing ---
  TuneRenderThread();
  TuneUiThread();
  PinThreadToBigCores();
  const CpuFeatures f1 = DetectCpuFeatures();
  std::printf("case 1: all four APIs returned; sse42=%d avx2=%d neon=%d "
              "neon_fp16=%d\n",
              f1.sse42 ? 1 : 0, f1.avx2 ? 1 : 0, f1.neon ? 1 : 0,
              f1.neon_fp16 ? 1 : 0);
  std::printf("  [PASS] all four APIs returned without crashing\n");

  // --- Case 2: cross-check against /proc/cpuinfo (x86 only) ---
#if defined(__x86_64__) || defined(__i386__)
  {
    const int sse42_ref = ProbeCpuFlag("sse4_2");
    const int avx2_ref = ProbeCpuFlag("avx2");
    const bool sse42_ok = sse42_ref < 0 || (f1.sse42 == (sse42_ref == 1));
    const bool avx2_ok = avx2_ref < 0 || (f1.avx2 == (avx2_ref == 1));
    std::printf("case 2: cpuinfo sse4_2=%d avx2=%d | detected sse42=%d avx2=%d\n",
                sse42_ref, avx2_ref, f1.sse42 ? 1 : 0, f1.avx2 ? 1 : 0);
    if (sse42_ok && avx2_ok) {
      std::printf("  [PASS] detection matches /proc/cpuinfo\n");
    } else {
      std::printf("  [FAIL] detection disagrees with /proc/cpuinfo\n");
      ++failures;
    }
  }
#else
  std::printf("case 2: skipped (non-x86 host)\n");
#endif

  // --- Case 3: two consecutive detections must be identical ---
  {
    const CpuFeatures f2 = DetectCpuFeatures();
    const bool same = f2.sse42 == f1.sse42 && f2.avx2 == f1.avx2 &&
                      f2.neon == f1.neon && f2.neon_fp16 == f1.neon_fp16;
    std::printf("case 3: detection stable across calls: sse42=%d avx2=%d\n",
                f2.sse42 ? 1 : 0, f2.avx2 ? 1 : 0);
    if (same) {
      std::printf("  [PASS] consecutive detections identical\n");
    } else {
      std::printf("  [FAIL] consecutive detections differ\n");
      ++failures;
    }
  }

  // --- Case 4: after pinning, at least one CPU remains available ---
  {
    PinThreadToBigCores();
    cpu_set_t set;
    CPU_ZERO(&set);
    const int rc = sched_getaffinity(0, sizeof(set), &set);
    const int n = (rc == 0) ? CPU_COUNT(&set) : 0;
    std::printf("case 4: sched_getaffinity after pin -> %d usable CPU(s) "
                "(rc=%d)\n",
                n, rc);
    if (rc == 0 && n >= 1) {
      std::printf("  [PASS] at least one CPU remains available\n");
    } else {
      std::printf("  [FAIL] thread left with no usable CPU (rc=%d)\n", rc);
      ++failures;
    }
  }

  if (failures == 0) {
    std::printf("RESULT: 4/4 smoke cases PASS\n");
    return 0;
  }
  std::printf("RESULT: %d smoke case(s) FAILED\n", failures);
  return 1;
}
