// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - native_tuning.h
//
// Platform-native tuning layer (src/native/). This is the ONLY place where
// framework code is allowed to call OS-specific low-level APIs directly.
//
// Scope (what belongs here):
//   - thread scheduling / priority shaping (render thread, UI thread)
//   - platform timer resolution (frame pacing)
//   - CPU feature detection for future SIMD paths
//
// Out of scope (what must NOT go here):
//   - any rendering or GPU API use (that lives behind tgfx; see the
//     NEOFLUX_BACKEND compile-time selection in thirdparty/CMakeLists.txt)
//   - media decoding / interop (that is the GL backend's media module)
//   - windowing (GLFW bridge / mobile bridge in src/renderers/)
//
// Every entry point is best-effort: on restricted systems (no privileges,
// exotic kernels, unsupported CPUs) they degrade to safe no-ops and log
// once, never throwing and never failing the caller.
// =============================================================================

#pragma once

namespace neoflux {
namespace native {

/// Bitmask-style snapshot of CPU SIMD capabilities relevant to hot paths.
struct CpuFeatures {
  bool sse42{false};      ///< x86/x64: SSE4.2
  bool avx2{false};       ///< x86/x64: AVX2 (hardware AND OS-enabled via XCR0)
  bool neon{false};       ///< ARM/ARM64: ASIMD
  bool neon_fp16{false};  ///< ARM64: FP16 arithmetic extension
};

/// Tune the calling thread for rendering (call at the top of the render
/// thread body, before any GPU work). Raises scheduling priority and/or
/// registers with the platform multimedia scheduler (MMCSS on Windows,
/// SCHED_FIFO on Linux, QoS class on Apple).
void TuneRenderThread();

/// Tune the calling thread for UI/event-loop duty (call at the start of
/// EventLoop::Run()). On Windows this also requests 1 ms timer resolution
/// and raises the whole process priority class: condition_variable::wait_for()
/// inherits the ~15.6 ms default timer granularity, which visibly jitters
/// frame pacing at 60 FPS.
void TuneUiThread();

/// Attempt to pin the calling thread to "big"/performance cores when the
/// platform exposes a big.LITTLE-style topology (Linux/Android via cpufreq
/// data, Windows via EfficiencyClass). No-op on homogeneous topologies,
/// when topology data is unavailable, or on Apple platforms where QoS
/// already drives cluster placement and affinity APIs are not honoured.
void PinThreadToBigCores();

/// Detect CPU SIMD features. Purely informational; callers must still
/// provide a scalar fallback (results are not cached across CPUs, hotplug
/// aside this is stable for the process lifetime).
CpuFeatures DetectCpuFeatures();

}  // namespace native
}  // namespace neoflux
