// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - platform_linux.cpp
//
// Linux (and Android) tuning: best-effort realtime scheduling for the render
// thread, nice bump for the UI thread, and CPU feature detection.
//
// This file contains no compiler intrinsic and no inline assembly. Everything
// that needs a CPU instruction goes through native/asm (see cpuid_bits.h and
// cache_topology_cpuid.h), so this is now the same detection code that runs on
// Windows, fed by the same assembly primitives. The only platform-specific part
// left is what the OPERATING SYSTEM reports: cpufreq frequencies for big-core
// pinning and HWCAP for the ARM feature bits.
// =============================================================================

// cpu_set_t / sched_setaffinity are GNU extensions: they disappear under
// strict -std=c++20 unless this is defined before any system header.
#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include "native/native_tuning.h"

#include <pthread.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <string>
#include <system_error>
#include <vector>

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

#include "neoflux/core/config.h"
#include "neoflux/core/flags.h"
#include "native/asm/asm_symbols.h"
#include "native/cache_topology_cpuid.h"
#include "native/cpuid_bits.h"

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

/// Reads XCR0 (OS-enabled extended register state).
///
/// The read needs the xgetbv instruction, so it comes from asm/xgetbv.S
/// whenever CMake linked that file. Without it this returns 0 and the caller
/// reports AVX2 as unsupported, which is the safe direction: advertising YMM
/// state the OS was never confirmed to save would corrupt neighbouring threads.
std::uint64_t ReadXcr0() {
#if defined(NEOFLUX_NATIVE_ASM_XGETBV)
  return neoflux_read_xcr0();
#else
  return 0;
#endif
}

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
/// silently accepting it. The read is a single small file, so what matters is
/// the open(), not the conversion.
std::vector<std::int64_t> ReadCoreMaxFrequencies() {
  static constexpr const char* kMaxFreqSuffix = "/cpufreq/cpuinfo_max_freq";
  std::vector<std::int64_t> freqs;
  for (int cpu = 0;; ++cpu) {
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
  const std::int64_t max_freq = *std::max_element(freqs.begin(), freqs.end());

  // "Big" = every core whose max frequency reaches
  // --native_bigcore_threshold_permille (default 950 = 95%) of the fastest
  // core. On big.LITTLE SoCs the LITTLE cluster sits at 60-80% of the big
  // cluster's clock, so the default separates them cleanly while tolerating
  // turbo variance. Integer permille math avoids any float rounding.
  const std::int64_t threshold_permille = std::clamp(
      static_cast<std::int64_t>(FLAGS_native_bigcore_threshold_permille),
      std::int64_t{500}, std::int64_t{1000});
  cpu_set_t big_set;
  CPU_ZERO(&big_set);
  int big_count = 0;
  for (std::size_t cpu = 0; cpu < freqs.size(); ++cpu) {
    if (cpu >= CPU_SETSIZE) {
      break;  // cpu_set_t is a fixed 1024-bit mask: never index out-of-bounds.
    }
    if (freqs[cpu] * threshold_permille >= max_freq * 1000) {
      CPU_SET(cpu, &big_set);
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
  CpuFeatures features;

  // x86: the same CPUID reads as Windows, through the same assembly.
  const auto leaf0 = cpuid_bits::Subleaf(0U, 0U);
  const std::uint32_t max_leaf = leaf0[0];
  if (max_leaf >= 1U) {
    const auto leaf1 = cpuid_bits::Subleaf(1U, 0U);
    features.sse42 = (leaf1[2] & kSse42Bit) != 0U;
    const bool os_xsave = (leaf1[2] & kOsxsaveBit) != 0U;
    if (os_xsave && max_leaf >= 7U) {
      const auto leaf7 = cpuid_bits::Subleaf(7U, 0U);
      if ((leaf7[1] & kAvx2Bit) != 0U) {
        const std::uint64_t xcr0 = ReadXcr0();
        features.avx2 = ((xcr0 & kXcr0XmmYmmMask) == kXcr0XmmYmmMask);
      }
    }
  }

#if defined(__aarch64__) || defined(__arm__)
  // ARM has no CPUID: the kernel publishes the feature bits through AT_HWCAP,
  // so these two fields come from the OS rather than from an instruction.
  const unsigned long hwcap = getauxval(AT_HWCAP);
  features.neon = (hwcap & HWCAP_ASIMD) != 0;
  features.neon_fp16 = (hwcap & (HWCAP_FPHP | HWCAP_ASIMDHP)) != 0;
#endif

  return features;
}

CpuFeatures DetectCpuFeatures() noexcept {
  // CPU features are a process-lifetime invariant: probe once, then serve
  // the cached snapshot. Magic static => thread-safe one-shot evaluation.
  static const CpuFeatures kCached = DetectCpuFeaturesImpl();
  return kCached;
}

CacheInfo DetectCacheTopology() noexcept {
  // x86 reports the cache hierarchy through CPUID leaf 4, exactly as Windows
  // does, so the shared walk is used rather than parsing sysfs: one decoder,
  // one set of bit masks to get right.
  //
  // ARM exposes no CPUID. The kernel does publish the hierarchy, but only as
  // sysfs text files, and Android's NDK does not guarantee the CPUID path even
  // on x86. So on a non-x86 target the documented fallback stands: line_size
  // 64 (which matches every ARM64 part in practice) and capacities 0 =
  // unknown. Guessing capacities from sysfs text would buy a log line and
  // nothing else.
#if defined(__x86_64__) || defined(__i386__)
  static const CacheInfo kCached = DetectCacheTopologyCpuid();
#else
  static const CacheInfo kCached = CacheInfo{};
#endif
  return kCached;
}

void PrefetchForRead(const void* p) noexcept {
#if defined(NEOFLUX_NATIVE_ASM_PREFETCH)
  neoflux_prefetch_read(p);
#else
  (void)p;
#endif
}

void PrefetchForWrite(const void* p) noexcept {
#if defined(NEOFLUX_NATIVE_ASM_PREFETCH)
  neoflux_prefetch_write(p);
#else
  (void)p;
#endif
}

void VerifyCacheLineConfig() noexcept {
  const CacheInfo info = DetectCacheTopology();
  LOG_FIRST_N(INFO, 1) << "native: cache topology detected -- line="
                       << info.line_size << "B L1d=" << info.l1d_bytes
                       << "B L2=" << info.l2_bytes << "B L3=" << info.l3_bytes
                       << "B";

  if (info.line_size > config::kCacheLineSize) {
    // The runtime coherence line is WIDER than the compile-time alignment:
    // objects padded to kCacheLineSize can still share a real cache line,
    // so false sharing is possible. Warn exactly once (LOG_FIRST_N) and tell
    // the user how to fix it.
    LOG_FIRST_N(WARNING, 1)
        << "native: runtime cache line (" << info.line_size
        << "B) exceeds compile-time config::kCacheLineSize ("
        << config::kCacheLineSize
        << "B); SPSC queue head/tail may still share a coherence line -> "
           "false sharing. Rebuild with -DNEOFLUX_CACHE_LINE_SIZE="
        << info.line_size << " to fix it.";
  } else if (info.line_size < config::kCacheLineSize) {
    // Oversized padding: correct but wasteful (a little memory), so info only.
    LOG_FIRST_N(INFO, 1)
        << "native: compile-time cache line (" << config::kCacheLineSize
        << "B) is more conservative than runtime (" << info.line_size
        << "B); alignment is safe.";
  } else {
    LOG_FIRST_N(INFO, 1) << "native: compile-time cache line matches runtime ("
                         << info.line_size << "B)";
  }
}

}  // namespace neoflux::native
