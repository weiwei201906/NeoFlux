// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - platform_win32.cpp
//
// Windows tuning: MMCSS registration / thread priority shaping, process
// priority class, 1 ms timer resolution for frame pacing, big-core pinning
// via EfficiencyClass, and x86 CPU feature detection via cpuid/xgetbv.
// =============================================================================

#include "native/native_tuning.h"

// neoflux_read_xcr0() (asm/xgetbv.S) is used by the non-MSVC x86_64 branches
// below. MSVC uses the _xgetbv intrinsic instead and does not link the .S.
#if !defined(_MSC_VER) && (defined(__x86_64__) || defined(_M_X64))
#include "native/asm/asm_symbols.h"
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
// EfficiencyClass in SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX and
// GetLogicalProcessorInformationEx both require targeting Win10 headers.
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <windows.h>

#include <avrt.h>
#include <intrin.h>

#include <bit>
#include <cstdint>
#include <vector>

#include <glog/logging.h>

#include "neoflux/core/flags.h"

namespace neoflux::native {
namespace {

// --- cpuid feature bits (CPUID leaf 1 ECX / leaf 7 EBX), named for clarity.
// A CPUID GPR is architecturally 32 bits wide, so these spell std::uint32_t
// instead of inheriting whatever width this compiler gives `unsigned int`.
constexpr std::uint32_t kSse42Bit = 1u << 20;    ///< leaf 1 ECX[20]
constexpr std::uint32_t kOsxsaveBit = 1u << 27;  ///< leaf 1 ECX[27]: XGETBV ok
constexpr std::uint32_t kAvx2Bit = 1u << 5;      ///< leaf 7 EBX[5]
// XCR0 bits [2:1] must both be set for the OS to save/restore YMM state.
// XCR0 is a 64-bit MSR, which is why this one is not 32 bits like the rest.
constexpr std::uint64_t kXcr0XmmYmmMask = 0x6ULL;

/// Reads XCR0 (OS-enabled extended register state) portably across MSVC and
/// GCC/Clang-on-Windows. Needed to confirm the OS saves/restores YMM before
/// advertising AVX2.
std::uint64_t ReadXcr0() {
#if defined(_MSC_VER) && !defined(__clang__)
  // _xgetbv is a compiler intrinsic, not inline asm -- allowed by the asm
  // policy (which only forbids __asm__/__asm in C++ sources).
  return _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
#else
  // GCC/Clang/MinGW (x86_64): the xgetbv instruction is implemented in
  // asm/xgetbv.S; inline asm is forbidden by project policy.
  return neoflux_read_xcr0();
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

  ~MmcssRegistration() {
    if (handle != nullptr) {
      // Revert only: the priority request was made right after registration;
      // the destructor's single job is to give the characteristics handle
      // back exactly once.
      AvRevertMmThreadCharacteristics(handle);
    }
  }
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
  if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL)) {
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
  if (!SetPriorityClass(GetCurrentProcess(),
                        ABOVE_NORMAL_PRIORITY_CLASS)) {
    LOG(WARNING) << "native: SetPriorityClass failed, err=" << GetLastError();
  }
  if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL)) {
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
  std::vector<char> buffer(size);
  auto* const info =
      reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data());
  if (!GetLogicalProcessorInformationEx(RelationProcessorCore, info, &size)) {
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
            buffer.data() + offset);
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
  if (!SetThreadAffinityMask(GetCurrentThread(), big_mask)) {
    LOG(WARNING) << "native: SetThreadAffinityMask failed, err="
                 << GetLastError();
    return;
  }
  LOG(INFO) << "native: thread pinned to "
            << std::popcount(static_cast<std::uint64_t>(big_mask))
            << " performance core(s)";
}

CpuFeatures DetectCpuFeaturesImpl() {
  CpuFeatures f;
  int regs[4] = {0, 0, 0, 0};

  // Highest CPUID leaf.
  __cpuid(regs, 0);
  const int max_leaf = regs[0];
  if (max_leaf < 1) {
    return f;  // Should be impossible on any x86 that boots Windows.
  }

  // Leaf 1: ECX bit 20 = SSE4.2, bit 27 = OSXSAVE (XGETBV usable).
  __cpuid(regs, 1);
  f.sse42 = (regs[2] & kSse42Bit) != 0;
  const bool os_xsave = (regs[2] & kOsxsaveBit) != 0;

  if (os_xsave && max_leaf >= 7) {
    // Leaf 7 subleaf 0: EBX bit 5 = AVX2. __cpuidex pins ECX to subleaf 0
    // instead of relying on whatever ECX the previous __cpuid left behind.
    __cpuidex(regs, 7, 0);
    if ((regs[1] & kAvx2Bit) != 0) {
      // The OS must save/restore YMM across context switches: XCR0 bits
      // [2:1] must both be set (XMM + YMM state enabled).
      const std::uint64_t xcr0 = ReadXcr0();
      f.avx2 = ((xcr0 & kXcr0XmmYmmMask) == kXcr0XmmYmmMask);
    }
  }
  return f;
}

CpuFeatures DetectCpuFeatures() noexcept {
  // CPU features are a process-lifetime invariant: probe once, then serve
  // the cached snapshot. Magic static => thread-safe one-shot evaluation.
  static const CpuFeatures kCached = DetectCpuFeaturesImpl();
  return kCached;
}

}  // namespace neoflux::native
