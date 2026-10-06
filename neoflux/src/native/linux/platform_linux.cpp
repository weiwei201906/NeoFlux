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
#include <cerrno>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <string>
#include <system_error>
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

#include "neoflux/core/flags.h"

namespace neoflux::native {
namespace {

// --- cpuid feature bits (CPUID leaf 1 ECX / leaf 7 EBX), named for clarity.
// Every CPUID GPR is architecturally 32 bits wide, so these say std::uint32_t
// rather than relying on how wide this host's `unsigned int` happens to be.
constexpr std::uint32_t kSse42Bit = 1U << 20;    ///< leaf 1 ECX[20]
constexpr std::uint32_t kOsxsaveBit = 1U << 27;  ///< leaf 1 ECX[27]: XGETBV ok
constexpr std::uint32_t kAvx2Bit = 1U << 5;      ///< leaf 7 EBX[5]
// XCR0 bits [2:1] must both be set for the OS to save/restore YMM state.
constexpr std::uint64_t kXcr0XmmYmmMask = 0x6ULL;

/// Attempts a nice bump to the (clamped) configured value. Returns 0 on
/// success, or the errno set by setpriority(). Negative nice values require
/// CAP_SYS_NICE -- commonly absent in desktop sessions, so failure here is
/// normal and the caller stays quiet about it.
int TryNiceBump(int nice_value) {
  if (setpriority(PRIO_PROCESS, 0, nice_value) == 0) {
    return 0;
  }
  return errno;
}

/// Returns max frequency (kHz) per logical CPU from cpufreq, or an empty
/// vector when the sysfs tree is unavailable (containers, some VMs, x86
/// servers with acpi-cpufreq disabled). Present on virtually all ARM SoCs
/// (big.LITTLE) and modern Intel/AMD hybrid parts.
///
/// Parsing goes through std::from_chars rather than std::fscanf: the sysfs
/// value has no locale, and from_chars also rejects trailing junk instead of
/// silently accepting it. The read itself is a single small file, so the cost
/// that matters is the open(), not the conversion.
std::vector<std::int64_t> ReadCoreMaxFrequencies() {
  static constexpr const char* kMaxFreqSuffix = "/cpufreq/cpuinfo_max_freq";
  std::vector<std::int64_t> freqs;
  for (int cpu = 0;; ++cpu) {
    // One-shot startup probe: readability beats hand-rolled C-string building.
    const std::string path =
        "/sys/devices/system/cpu/cpu" + std::to_string(cpu) + kMaxFreqSuffix;
    FILE* const fp = std::fopen(path.c_str(), "r");
    if (fp == nullptr) {
      break;  // cpuN does not exist (or no cpufreq): stop at first gap.
    }
    // sysfs reports a short decimal plus a newline; 32 bytes is generous.
    char text[32] = {};
    const std::size_t read = std::fread(text, 1, sizeof(text) - 1, fp);
    std::fclose(fp);
    if (read == 0) {
      break;
    }
    std::int64_t khz = 0;
    const char* const begin = text;
    const char* const end = text + read;
    const auto parsed = std::from_chars(begin, end, khz);
    if (parsed.ec != std::errc{} || parsed.ptr == begin) {
      break;
    }
    freqs.push_back(khz);
  }
  return freqs;
}

}  // namespace

void TuneRenderThread() noexcept {
  if (!FLAGS_native_tuning) {
    LOG_FIRST_N(INFO, 1) << "native: tuning disabled by --nonative_tuning";
    return;
  }
  // Preferred: hard realtime FIFO scheduling. The RT priority (default: the
  // lowest, 1) is configurable via --native_render_rt_priority. Requires
  // CAP_SYS_NICE -- typically absent, so this is expected to fall through.
  sched_param sp{};
  sp.sched_priority = std::clamp(FLAGS_native_render_rt_priority, 1, 99);
  const int rt_rc = pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
  if (rt_rc == 0) {
    LOG(INFO) << "native: render thread -> SCHED_FIFO prio="
              << sp.sched_priority;
    return;
  }
  const int nice_value = std::clamp(FLAGS_native_thread_nice, -20, 19);
  if (TryNiceBump(nice_value) == 0) {
    LOG(INFO) << "native: render thread -> nice " << nice_value
              << " (RT scheduling denied)";
    return;
  }
  LOG(INFO) << "native: render thread keeps default scheduling (no "
               "RT/nice privileges)";
}

void TuneUiThread() noexcept {
  if (!FLAGS_native_tuning) {
    return;  // Master switch off; the render-thread call already logged once.
  }
  // Linux/Android kernels run hrtimers; condition_variable::wait_for() is
  // already sub-millisecond accurate, so there is no timer-resolution work
  // to do here (unlike Windows). A gentle nice bump helps input latency when
  // compositing is busy; failures are normal for unprivileged sessions.
  const int nice_value = std::clamp(FLAGS_native_thread_nice, -20, 19);
  if (TryNiceBump(nice_value) == 0) {
    LOG(INFO) << "native: ui thread -> nice " << nice_value;
  }
}

void PinThreadToBigCores() noexcept {
  if (!FLAGS_native_tuning) {
    return;
  }
  const std::vector<std::int64_t> freqs = ReadCoreMaxFrequencies();
  if (freqs.size() < 2) {
    return;  // No topology data or single core: nothing to do, silently.
  }
  const std::int64_t max_freq =
      *std::max_element(freqs.begin(), freqs.end());

  // "Big" = every core whose max frequency reaches
  // --native_bigcore_threshold_permille (default 950 = 95%) of the fastest
  // core. On big.LITTLE SoCs the LITTLE cluster sits at 60-80% of the big
  // cluster's clock, so the default separates them cleanly while tolerating
  // turbo variance. Integer permille math avoids any float rounding.
  const std::int64_t threshold_permille =
      std::clamp(static_cast<std::int64_t>(FLAGS_native_bigcore_threshold_permille),
                 std::int64_t{500}, std::int64_t{1000});
  cpu_set_t big_set;
  CPU_ZERO(&big_set);
  int big_count = 0;
  for (std::size_t cpu = 0; cpu < freqs.size(); ++cpu) {
    if (cpu >= CPU_SETSIZE) {
      break;  // cpu_set_t is a fixed 1024-bit mask: never index out-of-bounds.
    }
    if (freqs[cpu] * threshold_permille >= max_freq * 1000) {
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
  // One GPR per CPUID output register; __get_cpuid() takes unsigned int*, and
  // std::uint32_t is that same type on every x86-64 SysV target.
  std::uint32_t eax = 0;
  std::uint32_t ebx = 0;
  std::uint32_t ecx = 0;
  std::uint32_t edx = 0;
  if (__get_cpuid(1, &eax, &ebx, &ecx, &edx) == 0) {
    return f;  // cpuid leaf 1 unsupported: keep the all-false snapshot.
  }
  f.sse42 = (ecx & kSse42Bit) != 0;
  const bool os_xsave = (ecx & kOsxsaveBit) != 0;
  const std::uint32_t max_leaf = __get_cpuid_max(0, nullptr);
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
