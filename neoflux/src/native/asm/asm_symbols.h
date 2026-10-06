// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - asm/asm_symbols.h
//
// C++-facing declarations for routines implemented in this directory's
// hand-written .S files. Include this instead of writing inline asm or using a
// compiler intrinsic at the call site (the project forbids both in C++ sources:
// inline asm outright, and SIMD/CPUID intrinsics outside the dispatch layer).
//
// Linkage note: the symbols below live in asm/*.S and are NOT self-contained
// inline code -- failing to link the matching .S makes this an undefined
// reference. Every implementation is selected by platform/compiler/architecture
// on the CMake side, so only the targets that need a given routine link its .S.
// The NEOFLUX_NATIVE_ASM_* macros are what callers test, and CMake defines them
// from the same predicate that adds the source, so availability and the
// definition cannot disagree.
// =============================================================================

#pragma once

#include <cstdint>

extern "C" {

// ---------------------------------------------------------------------------
// x86 / x86-64: CPUID queries
//
// Selected by NEOFLUX_NATIVE_ASM_CPUID, which CMake defines only for a
// non-MSVC x86 target. MSVC assembles MASM rather than GAS syntax, so an MSVC
// build uses its own intrinsic instead; see neoflux/src/native/cpuid_bits.h,
// the only place either spelling appears.
// ---------------------------------------------------------------------------
#if defined(NEOFLUX_NATIVE_ASM_CPUID)

/// Runs CPUID for `leaf` with subleaf 0 and writes EAX, EBX, ECX and EDX to
/// regs[0..3]. `regs` must point at four std::uint32_t and is caller-owned.
///
/// Defined in asm/cpuid_x86.S. x86-64 only (SysV AMD64 and Win64 ABIs); the
/// integer argument registers differ between the two and are normalized inside
/// the assembly. No failure mode: an unsupported leaf reports the highest
/// supported leaf (through leaf 0) or zeros, which every caller already handles.
void neoflux_cpuid(std::uint32_t leaf, std::uint32_t* regs) noexcept;

/// Runs CPUID for `leaf` with an explicit `subleaf`. Required for the leaves
/// that enumerate one entry per subleaf, notably leaf 4 (cache parameters) and
/// leaf 7 (extended features). Same contract as neoflux_cpuid().
void neoflux_cpuid_subleaf(std::uint32_t leaf, std::uint32_t subleaf,
                           std::uint32_t* regs) noexcept;

#endif  // NEOFLUX_NATIVE_ASM_CPUID

// ---------------------------------------------------------------------------
// x86 / x86-64: XCR0 read
// ---------------------------------------------------------------------------
#if defined(NEOFLUX_NATIVE_ASM_XGETBV)

/// Reads XCR0 (the OS-enabled extended register state bitmap) and returns it as
/// a 64-bit value: (EDX << 32) | EAX.
///
///   Defined in asm/xgetbv.S, linked by any non-MSVC x86 target. MSVC uses the
///   _xgetbv intrinsic instead and never references this symbol.
///
///   Returned bit layout: bit 0 = x87, bit 1 = SSE/XMM, bit 2 = AVX/YMM.
///   Callers must first confirm CPUID leaf 1 ECX[27] (OSXSAVE): reading XCR0
///   without OSXSAVE is undefined and can fault.
///
/// `noexcept` matches the caller contract: it is a pure register read that
/// cannot throw.
std::uint64_t neoflux_read_xcr0() noexcept;

#endif  // NEOFLUX_NATIVE_ASM_XGETBV

// ---------------------------------------------------------------------------
// Cache prefetch hints
//
// One architecture-neutral declaration pair, two implementations:
//   x86-64:  asm/prefetch_x86.S     (PREFETCHT0)
//   AArch64: asm/prefetch_aarch64.S (PRFM PSTL1KEEP / PSTL1STRM)
//
// Both are hints by architecture: they cannot fault and do not read memory in
// the architectural sense, so a null or unmapped address is well defined.
// ---------------------------------------------------------------------------
#if defined(NEOFLUX_NATIVE_ASM_PREFETCH)

/// Prefetches the cache line containing `p` for reading, into the innermost
/// cache, marking the line for retention. For read-mostly data that is about to
/// be consumed (for example a batch drained from the render queue).
void neoflux_prefetch_read(const void* p) noexcept;

/// Prefetches the cache line containing `p` with a hint that it is about to be
/// written and is not expected to be reused immediately.
void neoflux_prefetch_write(const void* p) noexcept;

#endif  // NEOFLUX_NATIVE_ASM_PREFETCH

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
