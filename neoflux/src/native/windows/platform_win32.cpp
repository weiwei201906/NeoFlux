// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - platform_win32.cpp
//
// Windows tuning: MMCSS registration / thread priority shaping, process
// priority class, 1 ms timer resolution for frame pacing, big-core pinning via
// EfficiencyClass, and CPU feature / cache-topology detection.
//
// This file contains no compiler intrinsic and no inline assembly. Everything
// that needs a CPU instruction goes through native/asm (see cpuid_bits.h and
// cache_topology_cpuid.h), which is also why the detection logic here is the
// same code that runs on Linux and macOS.
// =============================================================================

#include "native/native_tuning.h"

// windows.h first: mmsyscom.h (pulled in by mmsystem.h) uses UINT, DWORD and
// BYTE without defining them. MSVC's windows.h happens to include mmsystem.h
// itself; MinGW's does not, so the order matters here.
#include <windows.h>

#include <avrt.h>
#include <mmsystem.h>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <glog/logging.h>

#include "neoflux/core/config.h"
#include "neoflux/core/flags.h"
#include "native/asm/asm_symbols.h"
#include "native/cache_topology_cpuid.h"
#include "native/cpuid_bits.h"

namespace neoflux::native {
namespace {

// --- cpuid feature bits (CPUID leaf 1 ECX / leaf 7 EBX), named for clarity.
// A CPUID GPR is architecturally 32 bits wide, so these spell std::uint32_t
// instead of inheriting whatever width this compiler gives `unsigned int`.
constexpr std::uint32_t kSse42Bit = 1U << 20;    ///< leaf 1 ECX[20]
constexpr std::uint32_t kOsxsaveBit = 1U << 27;  ///< leaf 1 ECX[27]: XGETBV ok
constexpr std::uint32_t kAvx2Bit = 1U << 5;      ///< leaf 7 EBX[5]
// XCR0 bits [2:1] must both be set for the OS to save/restore YMM state.
// XCR0 is a 64-bit MSR, which is why this one is not 32 bits like the rest.
constexpr std::uint64_t kXcr0XmmYmmMask = 0x6ULL;

/// Reads XCR0 (the OS-enabled extended register state bitmap).
///
/// The read needs the xgetbv instruction, which is not expressible in portable
/// C++, so it comes from asm/xgetbv.S whenever CMake linked that file. MSVC
/// assembles MASM rather than GAS syntax, so an MSVC build has no .S to link
/// and this returns 0: the caller then reports AVX2 as unsupported. That is the
/// safe direction -- claiming OS support for YMM state that was never verified
/// is precisely the bug this check exists to prevent.
///
/// The caller must have confirmed leaf 1 ECX[27] (OSXSAVE) first; reading XCR0
/// without OSXSAVE is undefined.
std::uint64_t ReadXcr0() {
#if defined(NEOFLUX_NATIVE_ASM_XGETBV)
  return neoflux_read_xcr0();
#else
  return 0;
#endif
}

/// MMCSS registration state so the render thread's characteristics handle is
/// released exactly once at thread exit. AvSetMmThreadCharacteristicsW hands
/// the thread to the configured profile (--native_mmcss_profile, default
/// "Games"): GPU-preemption-aware multimedia scheduling, higher priority than
/// a plain ABOVE_NORMAL bump.
struct MmcssRegistration {
  HANDLE handle{nullptr};
  DWORD index{0};

  // Thread-local state, not a value: a copy would hand two objects the same
  // characteristics handle and revert it twice.
  MmcssRegistration() = default;
  ~MmcssRegistration() {
    if (handle != nullptr) {
      // Revert only: the priority request was made right after registration;
      // the destructor's single job is to give the characteristics handle
      // back exactly once.
      AvRevertMmThreadCharacteristics(handle);
    }
  }
  MmcssRegistration(const MmcssRegistration&) = delete;
  MmcssRegistration& operator=(const MmcssRegistration&) = delete;
  MmcssRegistration(MmcssRegistration&&) = delete;
  MmcssRegistration& operator=(MmcssRegistration&&) = delete;
};

}  // namespace

void TuneRenderThread() noexcept {
  if (!FLAGS_native_tuning) {
    LOG_FIRST_N(INFO, 1) << "native: tuning disabled by --nonative_tuning";
    return;
  }
  // Preferred: register with MMCSS. The profile (default "Games") comes from
  // --native_mmcss_profile. This supersedes a plain SetThreadPriority call:
  // the scheduler treats MMCSS threads with GPU-preemption awareness, which
  // is exactly the render thread's job.
  static thread_local MmcssRegistration mmcss;
  if (mmcss.handle != nullptr) {
    return;  // Idempotent: re-entering TuneRenderThread must not leak a
             // second characteristics handle by overwriting this one.
  }
  wchar_t profile[64];
  const int wlen = ::MultiByteToWideChar(
      CP_UTF8, 0, FLAGS_native_mmcss_profile.c_str(), -1, profile,
      static_cast<int>(sizeof(profile) / sizeof(profile[0])));
  mmcss.handle = (wlen > 0) ? AvSetMmThreadCharacteristicsW(profile, &mmcss.index)
                            : nullptr;
  if (mmcss.handle != nullptr) {
    // Request high MMCSS priority right away -- the destructor only reverts,
    // so this is the one and only place the priority is chosen.
    AvSetMmThreadPriority(mmcss.handle, AVRT_PRIORITY_HIGH);
    LOG(INFO) << "native: render thread registered with MMCSS profile '"
              << FLAGS_native_mmcss_profile << "' (priority HIGH)";
    return;
  }
  // Fallback: ABOVE_NORMAL rather than THREAD_PRIORITY_TIME_CRITICAL -- the
  // render thread is latency-sensitive, but TIME_CRITICAL on a non-realtime
  // OS can starve UI/input threads and cause worse perceived stutter.
  if (SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL) == 0) {
    LOG(WARNING) << "native: SetThreadPriority(render) failed, err="
                 << GetLastError();
    return;
  }
  LOG(INFO) << "native: render thread priority = ABOVE_NORMAL (MMCSS "
               "unavailable)";
}

void TuneUiThread() noexcept {
  if (!FLAGS_native_tuning) {
    return;  // Master switch off; the render-thread call already logged once.
  }
  // Frame pacing: EventLoop::Run() waits on a condition_variable with a
  // frame_duration timeout. CV waits inherit the system timer resolution,
  // which defaults to ~15.6 ms -- fatal for 60 FPS pacing. Request the best
  // resolution the platform grants us (--native_timer_period_ms, default
  // 1 ms; 0 disables the request). On Windows 10 2004+ this call is
  // automatically scoped to the calling process (pre-Win10 it is global, so
  // never request coarser than the default).
  const int period_ms = FLAGS_native_timer_period_ms;
  if (period_ms > 0) {
    const MMRESULT mmres = timeBeginPeriod(static_cast<UINT>(period_ms));
    if (mmres != TIMERR_NOERROR) {
      LOG(WARNING) << "native: timeBeginPeriod(" << period_ms
                   << ") failed, err=" << mmres;
    }
  }
  // Process-wide priority: keeps the whole app (UI, worker, render threads
  // created later) above the default class on a contended desktop without
  // reaching Realtime, which the scheduler punishes for I/O-heavy work.
  if (SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS) == 0) {
    LOG(WARNING) << "native: SetPriorityClass failed, err=" << GetLastError();
  }
  if (SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL) == 0) {
    LOG(WARNING) << "native: SetThreadPriority(ui) failed, err="
                 << GetLastError();
    return;
  }
  LOG(INFO) << "native: ui thread tuned (1 ms timer resolution + "
               "ABOVE_NORMAL process class)";
}

void PinThreadToBigCores() noexcept {
  // Ask for the full logical-processor topology in one shot.
  DWORD size = 0;
  GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &size);
  if (size == 0) {
    return;  // API unavailable / failed: stay unpinned.
  }
  // The API reports a byte count, but SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX
  // contains pointer-sized members, so the storage must carry
  // alignof(std::max_align_t) at least. std::vector<std::byte> guarantees
  // alignment 1 and reinterpreting its data() would be undefined behaviour on
  // targets where the base alignment really is 1. Backing the buffer with
  // max_align_t costs at most one element of slack and makes the cast
  // well-defined everywhere.
  const std::size_t slots =
      (static_cast<std::size_t>(size) + sizeof(std::max_align_t) - 1) /
      sizeof(std::max_align_t);
  std::vector<std::max_align_t> buffer(slots);
  auto* const base = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(
      buffer.data());
  if (GetLogicalProcessorInformationEx(RelationProcessorCore, base, &size) == 0) {
    return;
  }

  // EfficiencyClass > 0 marks P-cores on hybrid/Big.LITTLE parts
  // (Intel 12th-gen+, Snapdragon). Cores with class 0 are E-cores or a
  // homogeneous topology, where pinning would only reduce scheduler freedom.
  DWORD_PTR big_mask = 0;
  DWORD offset = 0;
  bool any_big = false;
  while (offset < size) {
    auto* const entry =
        reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(
            reinterpret_cast<std::byte*>(buffer.data()) + offset);
    if (entry->Relationship == RelationProcessorCore &&
        entry->Processor.EfficiencyClass > 0) {
      // GROUP_AFFINITY may span groups; this app does not support >64-core
      // groups yet -- take group 0 cores only.
      if (entry->Processor.GroupCount > 0) {
        const GROUP_AFFINITY& ga = entry->Processor.GroupMask[0];
        if (ga.Group == 0) {
          big_mask |= ga.Mask;
        }
      }
      any_big = true;
    }
    offset += entry->Size;
  }
  if (!any_big || big_mask == 0) {
    LOG(INFO) << "native: homogeneous topology detected, thread not pinned";
    return;
  }
  if (SetThreadAffinityMask(GetCurrentThread(), big_mask) == 0) {
    LOG(WARNING) << "native: SetThreadAffinityMask failed, err="
                 << GetLastError();
    return;
  }
  LOG(INFO) << "native: thread pinned to "
            << std::popcount(static_cast<std::uint64_t>(big_mask))
            << " performance core(s)";
}

void PrefetchForRead(const void* p) noexcept {
#if defined(NEOFLUX_NATIVE_ASM_PREFETCH)
  neoflux_prefetch_read(p);
#else
  (void)p;  // No assembler on this toolchain: the hint is simply not issued.
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
    LOG_FIRST_N(WARNING, 1)
        << "native: runtime cache line (" << info.line_size
        << "B) exceeds compile-time config::kCacheLineSize ("
        << config::kCacheLineSize
        << "B); SPSC queue head/tail may still share a coherence line -> "
           "false sharing. Rebuild with -DNEOFLUX_CACHE_LINE_SIZE="
        << info.line_size << " to fix it.";
  } else if (info.line_size < config::kCacheLineSize) {
    LOG_FIRST_N(INFO, 1)
        << "native: compile-time cache line (" << config::kCacheLineSize
        << "B) is more conservative than runtime (" << info.line_size
        << "B); alignment is safe.";
  } else {
    LOG_FIRST_N(INFO, 1) << "native: compile-time cache line matches runtime ("
                         << info.line_size << "B)";
  }
}

CpuFeatures DetectCpuFeaturesImpl() {
  CpuFeatures features;

  const auto leaf0 = cpuid_bits::Subleaf(0U, 0U);
  const std::uint32_t max_leaf = leaf0[0];
  if (max_leaf < 1U) {
    return features;  // Should be impossible on any x86 that boots Windows.
  }

  // Leaf 1: ECX bit 20 = SSE4.2, bit 27 = OSXSAVE (XGETBV usable).
  const auto leaf1 = cpuid_bits::Subleaf(1U, 0U);
  features.sse42 = (leaf1[2] & kSse42Bit) != 0U;
  const bool os_xsave = (leaf1[2] & kOsxsaveBit) != 0U;

  if (os_xsave && max_leaf >= 7U) {
    // Leaf 7 subleaf 0: EBX bit 5 = AVX2. The subleaf form pins ECX to 0
    // instead of relying on whatever the previous CPUID left behind.
    const auto leaf7 = cpuid_bits::Subleaf(7U, 0U);
    if ((leaf7[1] & kAvx2Bit) != 0U) {
      // The OS must save/restore YMM across context switches: XCR0 bits
      // [2:1] must both be set (XMM + YMM state enabled).
      const std::uint64_t xcr0 = ReadXcr0();
      features.avx2 = ((xcr0 & kXcr0XmmYmmMask) == kXcr0XmmYmmMask);
    }
  }
  return features;
}

CpuFeatures DetectCpuFeatures() noexcept {
  // CPU features are a process-lifetime invariant: probe once, then serve
  // the cached snapshot. Magic static => thread-safe one-shot evaluation.
  static const CpuFeatures kCached = DetectCpuFeaturesImpl();
  return kCached;
}

CacheInfo DetectCacheTopology() noexcept {
  // Same contract as DetectCpuFeatures(): probe once, serve the snapshot.
  static const CacheInfo kCached = DetectCacheTopologyCpuid();
  return kCached;
}

}  // namespace neoflux::native
