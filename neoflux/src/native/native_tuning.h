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
//   - any rendering or GPU API use (that lives behind tgfx; the GPU backend
//     is tgfx's own TGFX_USE_* compile-time choice, see thirdparty/CMakeLists.txt)
//   - media decoding / interop (that is the GL backend's media module)
//   - windowing (GLFW bridge / mobile bridge in src/renderers/)
//
// Every entry point is best-effort: on restricted systems (no privileges,
// exotic kernels, unsupported CPUs) they degrade to safe no-ops and log
// once, never throwing and never failing the caller.
// =============================================================================

#pragma once

#include <cstddef>

namespace neoflux::native {

/// Bitmask-style snapshot of CPU SIMD capabilities relevant to hot paths.
///
/// `avx2` is never reported from the hardware bit alone: the OS must also be
/// saving and restoring YMM state, which on x86 means reading XCR0. That read
/// is the xgetbv instruction from asm/xgetbv.S on GCC and Clang, and the
/// _xgetbv intrinsic on MSVC. A build that links no assembly therefore reports
/// avx2 = false rather than guessing -- see ReadXcr0() in
/// native/windows/platform_win32.cpp.
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
void TuneRenderThread() noexcept;

/// Tune the calling thread for UI/event-loop duty (call at the start of
/// EventLoop::Run()). On Windows this also requests 1 ms timer resolution
/// and raises the whole process priority class: condition_variable::wait_for()
/// inherits the ~15.6 ms default timer granularity, which visibly jitters
/// frame pacing at 60 FPS.
void TuneUiThread() noexcept;

/// Attempt to pin the calling thread to "big"/performance cores when the
/// platform exposes a big.LITTLE-style topology (Linux/Android via cpufreq
/// data, Windows via EfficiencyClass). No-op on homogeneous topologies,
/// when topology data is unavailable, or on Apple platforms where QoS
/// already drives cluster placement and affinity APIs are not honoured.
void PinThreadToBigCores() noexcept;

/// Detect CPU SIMD features. The result is a process-lifetime invariant, so
/// the first call probes the CPU and every later call returns the cached
/// snapshot (thread-safe: magic-static initialization). Callers must still
/// provide a scalar fallback for unsupported features.
CpuFeatures DetectCpuFeatures() noexcept;

/// Snapshot of the CPU cache hierarchy relevant to hit-rate tuning.
/// `line_size` is the L1D coherence line -- the number that governs false
/// sharing (compare neoflux::config::kCacheLineSize, which is compile-time).
/// Capacity fields are 0 when the platform does not expose them.
struct CacheInfo {
  std::size_t line_size{64};  ///< L1D coherence line in bytes.
  std::size_t l1d_bytes{0};   ///< L1 data cache size, 0 = unknown.
  std::size_t l2_bytes{0};    ///< Largest L2 size, 0 = unknown.
  std::size_t l3_bytes{0};    ///< Largest L3 size, 0 = unknown.
};

/// Probe the runtime cache topology. Like DetectCpuFeatures(), the result is
/// a process-lifetime invariant and is cached after the first call. Platforms
/// without topology data yield line_size=64 and 0 capacities (safe defaults).
CacheInfo DetectCacheTopology() noexcept;

/// Prefetch one cache line for read into the innermost cache (locality 3).
/// The call is assembly-backed when the target enables NEOFLUX_NATIVE_ASM_PREFETCH;
/// otherwise it is a deliberate no-op. Use sparingly, only where a measured
/// stall dominates (e.g. right before consuming a batch from the render queue).
void PrefetchForRead(const void* p) noexcept;

/// Prefetch one cache line for write (requests exclusive / RFO line).
void PrefetchForWrite(const void* p) noexcept;

/// Cross-checks the runtime coherence line size against the compile-time
/// neoflux::config::kCacheLineSize. Called once from TuneUiThread(): if the
/// runtime line is WIDER than the compile-time padding, SPSC queue head/tail
/// may still share a line (false sharing) and the warning tells the user the
/// -DNEOFLUX_CACHE_LINE_SIZE=<n> override. Logs at most once per process.
void VerifyCacheLineConfig() noexcept;

}  // namespace neoflux::native
