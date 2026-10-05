// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - asm/asm_symbols.h
//
// C++-facing declarations for routines implemented in this directory's
// hand-written .S files. Include this instead of writing inline asm at the
// call site (the project forbids inline asm in C++ sources).
//
// Linkage note: the symbols below live in asm/*.S and are NOT self-contained
// inline code -- failing to link the matching .S makes this an undefined
// reference. Every implementation is selected by platform/compiler on the
// CMake side, so only the platforms that need a given routine link its .S.
// =============================================================================

#pragma once

#include <cstdint>

extern "C" {

/// Reads XCR0 (OS-enabled extended register state) and returns it as a
/// 64-bit value.
///
///   Defined in asm/xgetbv.S. That file is x86-64 only (SysV AMD64 and Win64
///   ABIs) and is linked ONLY for non-MSVC x86_64 builds -- i.e. Linux/macOS
///   GCC/Clang and MinGW/Clang-on-Windows. MSVC uses the _xgetbv intrinsic
///   directly in platform_win32.cpp and never references this symbol.
///
///   Returned bit layout: bit 0 = x87, bit 1 = SSE/XMM, bit 2 = AVX/YMM.
///   `noexcept` matches the caller contract: it is a pure register read that
///   cannot throw.
std::uint64_t neoflux_read_xcr0() noexcept;

// ---------------------------------------------------------------------------
// SIMD kernels
//
// The symbols in this section are the project's hand-written SIMD kernels.
// They exist as assembly rather than intrinsics because the point is
// instruction determinism, exact register control and cross-compiler
// consistency: intrinsics let each compiler pick different instructions and
// different register allocation, which is exactly what a kernel that has to
// behave identically on GCC, Clang, MSVC and AppleClang cannot accept.
//
// Declarations are guarded by NEOFLUX_NATIVE_ASM_PREMULTIPLY, defined by
// CMake only when the matching .S was actually added to the target. That
// keeps builds without an assembler (or with MSVC, which needs MASM rather
// than GAS syntax) from forming a reference to a symbol that was never
// compiled -- such a build fails at LINK time, and no amount of runtime
// guarding in the caller can prevent that.
// ---------------------------------------------------------------------------
#if defined(NEOFLUX_NATIVE_ASM_PREMULTIPLY_X86_64) || \
    defined(NEOFLUX_NATIVE_ASM_PREMULTIPLY_AARCH64)
// Derived, not defined by CMake: CMake sets exactly one architecture macro
// and the umbrella follows from it. Two independent switches would be two
// chances to disagree, and a disagreement here is a link error.
#define NEOFLUX_NATIVE_ASM_PREMULTIPLY 1
#endif

#if defined(NEOFLUX_NATIVE_ASM_PREMULTIPLY)

/// Converts straight RGBA8 to premultiplied RGBA8:
///
///   dst[c] = (src[c] * src[a] + 127) / 255   for c in {R, G, B}
///   dst[a] = src[a]
///
/// The divide is exact, not approximate: with t = channel * alpha + 127,
/// (t + (t >> 8)) >> 8 equals floor(t / 255) over the whole input range.
///
/// Defined by exactly one of, depending on the target CMake selected:
///   - asm/premultiply_rgba_x86_64.S   (SSE2,    4 pixels per pass)
///   - asm/premultiply_rgba_aarch64.S  (NEON,    8 pixels per pass)
/// Both are architectural baselines of their ISA, so neither needs a runtime
/// CPU feature probe.
///
/// Returns the number of pixels converted: floor(pixels / N) * N for the
/// pass size N of the linked kernel. Pixels past that count are left
/// completely untouched -- deliberately, so the caller can finish the tail
/// with the portable scalar kernel without double-converting anything.
///
/// |dst| may alias |src|.
///
/// Callers should use neoflux::native::PremultiplyRgba8() instead: it applies
/// the null/zero guards and finishes the tail. Calling this directly is only
/// appropriate from the dispatch layer and from tests that specifically want
/// to compare the assembly against a reference.
std::size_t neoflux_premultiply_rgba8(std::uint8_t* dst,
                                      const std::uint8_t* src,
                                      std::size_t pixels) noexcept;

#endif  // NEOFLUX_NATIVE_ASM_PREMULTIPLY

}  // extern "C"
