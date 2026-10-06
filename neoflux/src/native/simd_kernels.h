// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - native/simd_kernels.h
//
// C++ dispatch layer for the hand-written SIMD kernels that live in
// src/native/asm.
//
// This is the only place that knows which instruction set is in play, and it
// knows it at compile time. Callers outside src/native see a single portable
// function and never include an intrinsic header, never name an instruction
// set, and never touch the C ABI symbols. That is the whole point of the
// policy in docs/: business, render-pipeline and scheduling code must be able
// to call PremultiplyRgba8() without any of them learning that SSE2 or NEON
// exists.
//
// Why no runtime CPU feature detection
// ------------------------------------
// DetectCpuFeatures() (see native/native_tuning.h) exists, and a kernel that
// needs AVX2 or SVE2 would have to consult it, because those are optional
// extensions that an OS can also refuse to enable (XCR0 on x86, see
// asm/xgetbv.S). The two kernels shipped here are different: SSE2 is an
// architectural baseline of x86-64 and Advanced SIMD (NEON) is an
// architectural baseline of AArch64. Every implementation of those targets
// guarantees them, so a runtime probe would have nothing to decide and would
// only add a branch to a hot path. Should a non-baseline kernel be added
// later, it belongs here too, gated on DetectCpuFeatures() at runtime.
//
// This header is internal to the framework (src/, not include/): only the
// neoflux target and the standalone verify/ suite compile against it.
// =============================================================================

#pragma once

#include <cstddef>
#include <cstdint>

namespace neoflux::native {

/// Which kernel this build dispatches to. kScalar means no assembly was
/// compiled in and the portable C kernel does all the work.
///
/// Logged at start-up and asserted on by tests that want to prove a specific
/// path ran. Not a tuning knob: the value is fixed at compile time.
enum class SimdLevel : std::uint8_t {
  kScalar = 0,
  kSse2 = 1,
  kNeon = 2,
};

/// Converts |pixels| straight (non-premultiplied) RGBA8 pixels from |src|
/// into premultiplied RGBA8 in |dst|, four channels per pixel:
///
///   dst[c] = (src[c] * src[a] + 127) / 255   for c in {R, G, B}
///   dst[a] = src[a]
///
/// |dst| and |src| may alias.
///
/// No-op when |dst| or |src| is null, or when |pixels| is zero. Those guards
/// belong to the wrapper on purpose: the raw C ABI kernel promises only to
/// mask the pixel count before touching memory, and requiring every caller to
/// remember that is how null dereferences get shipped.
///
/// Thread safety: reads and writes only the buffers passed in, so it is safe
/// to call concurrently on disjoint buffers.
///
/// Cost: one pass of the active SIMD kernel over floor(pixels / N) * N pixels
/// plus a scalar tail of at most N - 1 pixels, where N is SimdGroupPixels().
void PremultiplyRgba8(std::uint8_t* dst, const std::uint8_t* src,
                      std::size_t pixels);

/// Returns the kernel this build dispatches to.
[[nodiscard]] SimdLevel ActiveSimdLevel() noexcept;

/// Pixels consumed per pass of the active kernel: 4 under SSE2, 8 under NEON,
/// 0 when no assembly was compiled in (kScalar). Equivalently, one more than
/// the largest tail PremultiplyRgba8() ever finishes with the scalar kernel.
[[nodiscard]] std::size_t SimdGroupPixels() noexcept;

}  // namespace neoflux::native
