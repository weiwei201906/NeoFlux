// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - platform_linux.cpp
//
// Linux (and Android) tuning: best-effort realtime scheduling for the render
// thread, nice bump for the UI thread, and CPU feature detection via
// cpuid (x86) / getauxval(AT_HWCAP) (ARM).
// =============================================================================

// cpu_set_t / sched_setaffinity are GNU extensions: they disappear under
// strict -std=c++20 unless this is defined before any system header.
#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include "native/native_tuning.h"

#if defined(__x86_64__)
// neoflux_read_xcr0() lives in asm/xgetbv.S (project policy: no inline asm in
// C++ sources). The .S is only linked for non-MSVC x86_64 builds, which is
// every x86_64 build on this platform.
#include "native/asm/asm_symbols.h"
#endif

#include <pthread.h>
#include <sched.h>
#include <sys/resource.h>
#include <unistd.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#endif

#if defined(__aarch64__) || defined(__arm__)
#include <sys/auxv.h>
// Keep local fallbacks: <asm/hwcap.h> availability differs across NDK/glibc
// versions and the bit assignments are part of the stable kernel UAPI.
#ifndef HWCAP_ASIMD
#define HWCAP_ASIMD (1UL << 1)
#endif
#ifndef HWCAP_FPHP
#define HWCAP_FPHP (1UL << 9)
#endif
#ifndef HWCAP_ASIMDHP
#define HWCAP_ASIMDHP (1UL << 10)
#endif
#endif

#include <glog/logging.h>

namespace neoflux::native {
namespace {

// --- cpuid feature bits (CPUID leaf 1 ECX / leaf 7 EBX), named for clarity.
constexpr unsigned int kSse42Bit = 1U << 20;    ///< leaf 1 ECX[20]
constexpr unsigned int kOsxsaveBit = 1U << 27;  ///< leaf 1 ECX[27]: XGETBV ok
constexpr unsigned int kAvx2Bit = 1U << 5;      ///< leaf 7 EBX[5]
// XCR0 bits [2:1] must both be set for the OS to save/restore YMM state.
constexpr std::uint64_t kXcr0XmmYmmMask = 0x6ULL;

/// Attempts a one-notch priority bump; returns true on success. Negative
/// nice values require CAP_SYS_NICE -- commonly absent in desktop sessions,
/// so failure here is normal and quiet.
bool TryNiceBump() {
  return setpriority(PRIO_PROCESS, 0, -5) == 0;
}

/// Returns max frequency (kHz) per logical CPU from cpufreq, or an empty
/// vector when the sysfs tree is unavailable (containers, some VMs, x86
/// servers with acpi-cpufreq disabled). Present on virtually all ARM SoCs
/// (big.LITTLE) and modern Intel/AMD hybrid parts.
std::vector<std::int64_t> ReadCoreMaxFrequencies() {
  static constexpr const char* kMaxFreqSuffix = "/cpufreq/cpuinfo_max_freq";
  std::vector<std::int64_t> freqs;
  for (int cpu = 0;; ++cpu) {
    // One-shot startup probe: readability beats the snprintf micro-cost.
    const std::string path =
        "/sys/devices/system/cpu/cpu" + std::to_string(cpu) + kMaxFreqSuffix;
    FILE* fp = std::fopen(path.c_str(), "r");
    if (fp == nullptr) {
      break;  // cpuN does not exist (or no cpufreq): stop at first gap.
    }
    std::int64_t khz = 0;
    const bool ok = std::fscanf(fp, "%ld", &khz) == 1;
    std::fclose(fp);
    if (!ok) {
      break;
    }
    freqs.push_back(khz);
  }
  return freqs;
}

}  // namespace

void TuneRenderThread() noexcept {
  // Preferred: hard realtime FIFO scheduling (lowest RT priority: we want
  // deadline predictability, not to outrank kernel threads). Requires
  // CAP_SYS_NICE -- typically absent, so this is expected to fall through.
  sched_param sp{};
  sp.sched_priority = 1;
  const int rt_rc = pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
  if (rt_rc == 0) {
    LOG(INFO) << "native: render thread -> SCHED_FIFO prio=1";
    return;
  }
  if (TryNiceBump()) {
    LOG(INFO) << "native: render thread -> nice -5 (RT scheduling denied)";
    return;
  }
  LOG(INFO) << "native: render thread keeps default scheduling (no "
               "RT/nice privileges)";
}

void TuneUiThread() noexcept {
  // Linux/Android kernels run hrtimers; condition_variable::wait_for() is
  // already sub-millisecond accurate, so there is no timer-resolution work
  // to do here (unlike Windows). A gentle nice bump helps input latency when
  // compositing is busy; failures are normal for unprivileged sessions.
  if (TryNiceBump()) {
    LOG(INFO) << "native: ui thread -> nice -5";
  }
}

void PinThreadToBigCores() noexcept {
  const std::vector<std::int64_t> freqs = ReadCoreMaxFrequencies();
  if (freqs.size() < 2) {
    return;  // No topology data or single core: nothing to do, silently.
  }
  const std::int64_t max_freq =
      *std::max_element(freqs.begin(), freqs.end());

  // "Big" = every core within 5% of the top frequency. On big.LITTLE SoCs
  // the LITTLE cluster sits at 60-80% of the big cluster's clock, so the
  // threshold separates them cleanly while tolerating turbo variance.
  cpu_set_t big_set;
  CPU_ZERO(&big_set);
  int big_count = 0;
  // Big core: within 5% of the top frequency (19/20 threshold). On
  // big.LITTLE SoCs the LITTLE cluster sits at 60-80% of the big cluster's
  // max clock, so this separates the clusters cleanly while tolerating
  // turbo variance. Integer math avoids any float rounding.
  constexpr std::int64_t kBigNum = 19;  // numerator of the 0.95 threshold
  constexpr std::int64_t kBigDen = 20;  // denominator
  for (size_t cpu = 0; cpu < freqs.size(); ++cpu) {
    if (cpu >= CPU_SETSIZE) {
      break;  // cpu_set_t is a fixed 1024-bit mask: never index out-of-bounds.
    }
    if (freqs[cpu] * kBigNum >= max_freq * kBigDen) {
      CPU_SET(static_cast<int>(cpu), &big_set);
      ++big_count;
    }
  }
  if (big_count == 0 || big_count == static_cast<int>(freqs.size())) {
    // Homogeneous topology (all cores same max clock): leave the scheduler
    // in charge -- pinning would reduce freedom with no upside.
    LOG(INFO) << "native: homogeneous cpu topology (" << freqs.size()
              << " cores), thread not pinned";
    return;
  }
  if (sched_setaffinity(0, sizeof(big_set), &big_set) != 0) {
    // EPERM without CAP_SYS_NICE on some systems: normal, stay unpinned.
    return;
  }
  LOG(INFO) << "native: render thread pinned to " << big_count << " big"
            << " core(s) of " << freqs.size();
}

CpuFeatures DetectCpuFeaturesImpl() {
  CpuFeatures f;

#if defined(__x86_64__)
  unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
  if (__get_cpuid(1, &eax, &ebx, &ecx, &edx) == 0) {
    return f;  // cpuid leaf 1 unsupported: keep the all-false snapshot.
  }
  f.sse42 = (ecx & kSse42Bit) != 0;
  const bool os_xsave = (ecx & kOsxsaveBit) != 0;
  unsigned int max_leaf = __get_cpuid_max(0, nullptr);
  if (os_xsave && max_leaf >= 7 &&
      __get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx) != 0) {
    if ((ebx & kAvx2Bit) != 0) {  // AVX2
      // neoflux_read_xcr0() lives in asm/xgetbv.S: pure register read, no
      // args, returns EDX:EAX as a 64-bit value (ABI-agnostic).
      const std::uint64_t xcr0 = neoflux_read_xcr0();
      f.avx2 = ((xcr0 & kXcr0XmmYmmMask) == kXcr0XmmYmmMask);
    }
  }
#elif defined(__aarch64__) || defined(__arm__)
  const unsigned long hwcap = getauxval(AT_HWCAP);
  f.neon = (hwcap & HWCAP_ASIMD) != 0;
  f.neon_fp16 = (hwcap & (HWCAP_FPHP | HWCAP_ASIMDHP)) != 0;
#endif

  return f;
}

CpuFeatures DetectCpuFeatures() noexcept {
  // CPU features are a process-lifetime invariant: probe once, then serve
  // the cached snapshot. Magic static => thread-safe one-shot evaluation.
  static const CpuFeatures kCached = DetectCpuFeaturesImpl();
  return kCached;
}

}  // namespace neoflux::native
