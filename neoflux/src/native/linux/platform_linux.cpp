// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - platform_linux.cpp
//
// Linux (and Android) tuning: best-effort realtime scheduling for the render
// thread, nice bump for the UI thread, and CPU feature detection via
// cpuid (x86) / getauxval(AT_HWCAP) (ARM).
// =============================================================================

#include "native/native_tuning.h"

#include <pthread.h>
#include <sched.h>
#include <sys/resource.h>
#include <unistd.h>

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

namespace neoflux {
namespace native {
namespace {

#if defined(__x86_64__) || defined(__i386__)
/// Reads XCR0 to confirm the OS enables YMM state (required for AVX2).
unsigned long long ReadXcr0() {
  unsigned int eax = 0, edx = 0;
  __asm__ volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(0));
  return (static_cast<unsigned long long>(edx) << 32) | eax;
}
#endif

/// Attempts a one-notch priority bump; returns true on success. Negative
/// nice values require CAP_SYS_NICE -- commonly absent in desktop sessions,
/// so failure here is normal and quiet.
bool TryNiceBump() {
  return setpriority(PRIO_PROCESS, 0, -5) == 0;
}

}  // namespace

void TuneRenderThread() {
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

void TuneUiThread() {
  // Linux/Android kernels run hrtimers; condition_variable::wait_for() is
  // already sub-millisecond accurate, so there is no timer-resolution work
  // to do here (unlike Windows). A gentle nice bump helps input latency when
  // compositing is busy; failures are normal for unprivileged sessions.
  if (TryNiceBump()) {
    LOG(INFO) << "native: ui thread -> nice -5";
  }
}

CpuFeatures DetectCpuFeatures() {
  CpuFeatures f;

#if defined(__x86_64__) || defined(__i386__)
  unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
  if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
    return f;
  }
  f.sse42 = (ecx & (1u << 20)) != 0;
  const bool os_xsave = (ecx & (1u << 27)) != 0;
  unsigned int max_leaf = __get_cpuid_max(0, nullptr);
  if (os_xsave && max_leaf >= 7 &&
      __get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
    if ((ebx & (1u << 5)) != 0) {  // AVX2
      const unsigned long long xcr0 = ReadXcr0();
      f.avx2 = ((xcr0 & 0x6ULL) == 0x6ULL);  // XMM+YMM state OS-enabled
    }
  }
#elif defined(__aarch64__) || defined(__arm__)
  const unsigned long hwcap = getauxval(AT_HWCAP);
  f.neon = (hwcap & HWCAP_ASIMD) != 0;
  f.neon_fp16 = (hwcap & (HWCAP_FPHP | HWCAP_ASIMDHP)) != 0;
#endif

  return f;
}

}  // namespace native
}  // namespace neoflux
