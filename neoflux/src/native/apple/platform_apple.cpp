// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - platform_apple.cpp
//
// macOS / iOS tuning: QoS class shaping for the render thread and CPU
// feature detection via sysctl (Intel) / mandatory-feature assumptions
// (Apple Silicon).
// =============================================================================

#include "native/native_tuning.h"

#include <pthread.h>
#include <sys/qos.h>
#include <sys/sysctl.h>

#include <glog/logging.h>

#include "neoflux/core/flags.h"

namespace neoflux::native {

void TuneRenderThread() noexcept {
  if (!FLAGS_native_tuning) {
    LOG_FIRST_N(INFO, 1) << "native: tuning disabled by --nonative_tuning";
    return;
  }
  // USER_INTERACTIVE is the top QoS tier: mapped by the scheduler to high
  // CPU priority with timers coalescing disabled. Exactly right for a
  // render thread pacing to the display refresh. Requires macOS 10.10+ /
  // iOS 8+ (deployment floor of the framework anyway).
  const int rc = pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
  if (rc != 0) {
    LOG(WARNING) << "native: pthread_set_qos_class_self_np failed, rc=" << rc;
    return;
  }
  LOG(INFO) << "native: render thread QoS = USER_INTERACTIVE";
}

void TuneUiThread() noexcept {
  if (!FLAGS_native_tuning) {
    return;  // Master switch off; the render-thread call already logged once.
  }
  // The main thread on macOS/iOS already runs at USER_INTERACTIVE QoS by
  // default, and timer resolution is not controllable from userspace.
  // Nothing to improve; kept as an explicit no-op so callers stay uniform.
  LOG(INFO) << "native: ui thread keeps default QoS (already USER_INTERACTIVE)";
}

void PinThreadToBigCores() noexcept {
  // Apple Silicon runs P/E clusters, but userspace has no stable API to
  // address them: thread affinity is not honoured on iOS and only has an
  // undocumented, App-Store-discouraged tag on macOS. The scheduler places
  // threads on P-cores based on their QoS class, which TuneRenderThread()
  // already set to USER_INTERACTIVE -- the correct mechanism on this
  // platform. Kept as an explicit no-op so callers stay uniform.
  LOG(INFO) << "native: core pinning handled by QoS on Apple platforms";
}

CpuFeatures DetectCpuFeaturesImpl() {
  CpuFeatures f;

#if defined(__aarch64__) || defined(__arm64__)
  // Apple Silicon mandates FEAT_FP16 and ASIMD as part of the arm64 ABI the
  // OS itself targets; there is no sysctl toggle to reflect.
  f.neon = true;
  int fp16 = 0;
  size_t len = sizeof(fp16);
  if (sysctlbyname("hw.optional.arm.FEAT_FP16", &fp16, &len, nullptr, 0) == 0) {
    f.neon_fp16 = fp16 != 0;
  }
#else
  int value = 0;
  size_t len = sizeof(value);
  if (sysctlbyname("hw.optional.sse4_2", &value, &len, nullptr, 0) == 0) {
    f.sse42 = value != 0;
  }
  value = 0;
  len = sizeof(value);
  if (sysctlbyname("hw.optional.avx2_0", &value, &len, nullptr, 0) == 0) {
    f.avx2 = value != 0;
  }
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
