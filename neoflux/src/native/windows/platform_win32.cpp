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
#include <vector>

#include <glog/logging.h>

namespace neoflux::native {
namespace {

// --- cpuid feature bits (CPUID leaf 1 ECX / leaf 7 EBX), named for clarity.
constexpr unsigned int kSse42Bit = 1u << 20;    ///< leaf 1 ECX[20]
constexpr unsigned int kOsxsaveBit = 1u << 27;  ///< leaf 1 ECX[27]: XGETBV ok
constexpr unsigned int kAvx2Bit = 1u << 5;      ///< leaf 7 EBX[5]
// XCR0 bits [2:1] must both be set for the OS to save/restore YMM state.
constexpr unsigned long long kXcr0XmmYmmMask = 0x6ULL;

/// Reads XCR0 (OS-enabled extended register state) portably across MSVC and
/// GCC/Clang-on-Windows. Needed to confirm the OS saves/restores YMM before
/// advertising AVX2.
unsigned long long ReadXcr0() {
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
/// the thread to the "Games" profile: GPU-preemption-aware multimedia
/// scheduling, higher priority than a plain ABOVE_NORMAL bump.
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
  // Preferred: register with MMCSS ("Games" profile). This supersedes a
  // plain SetThreadPriority call: the scheduler treats MMCSS threads with
  // GPU-preemption awareness, which is exactly the render thread's job.
  static thread_local MmcssRegistration mmcss;
  if (mmcss.handle != nullptr) {
    return;  // Idempotent: re-entering TuneRenderThread must not leak a
             // second characteristics handle by overwriting this one.
  }
  mmcss.handle = AvSetMmThreadCharacteristicsW(L"Games", &mmcss.index);
  if (mmcss.handle != nullptr) {
    // Request high MMCSS priority right away -- the destructor only reverts,
    // so this is the one and only place the priority is chosen.
    AvSetMmThreadPriority(mmcss.handle, AVRT_PRIORITY_HIGH);
    LOG(INFO) << "native: render thread registered with MMCSS profile "
                 "'Games' (priority HIGH)";
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
  // Frame pacing: EventLoop::Run() waits on a condition_variable with a
  // frame_duration timeout. CV waits inherit the system timer resolution,
  // which defaults to ~15.6 ms -- fatal for 60 FPS pacing. Request the best
  // resolution the platform grants us. On Windows 10 2004+ this call is
  // automatically scoped to the calling process (pre-Win10 it is global, so
  // keep the request at exactly 1 ms and never lower it).
  const MMRESULT mmres = timeBeginPeriod(1);
  if (mmres != TIMERR_NOERROR) {
    LOG(WARNING) << "native: timeBeginPeriod(1) failed, err=" << mmres;
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
  auto* info =
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
    auto* entry =
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
            << std::popcount(static_cast<unsigned long long>(big_mask))
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
      const unsigned long long xcr0 = ReadXcr0();
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
