// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - asm/premultiply_rgba_aarch64.S
//
// Contract
// --------
//   size_t neoflux_premultiply_rgba8(uint8_t* dst, const uint8_t* src,
//                                    size_t pixels);
//
//   Identical to asm/premultiply_rgba_x86_64.S, including the return value
//   (floor(pixels / 8) * 8 here, because one NEON pass handles 8 pixels) and
//   the "pixels beyond the count are untouched" guarantee. The two files are
//   selected per target by CMake; only one is ever linked into a build, so
//   the shared symbol name is deliberate and lets the dispatch layer stay
//   architecture-agnostic.
//
//     dst[c] = (src[c] * src[a] + 127) / 255    for c in {R, G, B}
//     dst[a] = src[a]
//
//   The division by 255 is exact: with t = channel * alpha + 127 (max 65152),
//   (t + (t >> 8)) >> 8 equals floor(t / 255) for every t in [0, 65152], and
//   every intermediate stays below 65406 so the 16-bit lanes never overflow.
//
// Instruction set
// ---------------
//   NEON / ASIMD only. Advanced SIMD is an architectural baseline of AArch64
//   (every AArch64 implementation guarantees it), so there is no getauxval /
//   HWCAP probe here. A future SVE2 kernel would NOT be a baseline and would
//   have to be gated on HWCAP_SVE at runtime.
//
// ABI notes (AAPCS64)
// -------------------
//   - Arguments: X0 = dst, X1 = src, X2 = pixels. Return value in X0.
//   - Callee-saved GPRs are X19-X28 (and X29/X30). This kernel uses X3 and X9
//     only, both caller-saved, so nothing needs saving or restoring.
//   - Callee-saved vector registers are the LOW 64 BITS of V8-V15. Everything
//     else (V0-V7 and V16-V31) is caller-saved. This kernel uses V0-V7 plus
//     V16 and V17 and therefore touches no callee-saved state at all. Using
//     V8-V15 here would corrupt the caller on AArch64 exactly the way using
//     XMM6-XMM15 would on Win64.
//   - X0 is advanced by the post-indexed ST4, so the return value is staged in
//     X3 and moved into X0 at the end.
//
// AArch64 only
// ------------
//   Selected by CMake on AArch64 targets (Linux/arm64, Android/arm64-v8a,
//   Apple silicon). No 32-bit ARM (AArch32) support is attempted: the
//   register widths and the LD4/ST4 element forms below are 64-bit specific.
//
// Apple symbol mangling
// ---------------------
//   On Mach-O (Apple silicon) the C symbol is emitted with a leading
//   underscore. SYMBOL_NAME below hides that difference.
// =============================================================================

#include "abi_macros.inc"

    .text

// -----------------------------------------------------------------------------
// size_t neoflux_premultiply_rgba8(uint8_t* dst, const uint8_t* src,
//                                  size_t pixels)
// -----------------------------------------------------------------------------
    .globl SYMBOL_NAME(neoflux_premultiply_rgba8)
    .p2align 2                                     // 4-byte align (instruction alignment)
#ifdef NF_ELF
    NF_ELF_FUNCBEGIN(neoflux_premultiply_rgba8)
#endif
SYMBOL_NAME(neoflux_premultiply_rgba8):
    movi     v16.8h, #127                          // v16 = rounding term, 8 lanes of 127

    // ---- Return value: floor(pixels / 8) * 8, staged in X3 for the whole call.
    lsr      x3, x2, #3                            // x3 = pixels / 8
    lsl      x3, x3, #3                            // x3 = (pixels / 8) * 8  (mask off the tail)
    mov      x9, x3                                // x9 = pixels still to convert
    cbz      x9, .Ldone                            // count < 8: nothing to do

.Lloop:
    // ---- Load 8 pixels de-interleaved: one vector per channel plane.
    ld4      {v0.8b, v1.8b, v2.8b, v3.8b}, [x1], #32   // v0=R v1=G v2=B v3=A, post-increment

    // ---- Widen each plane to 16-bit lanes so the multiply cannot overflow.
    uxtl     v4.8h, v0.8b                          // v4 = R widened
    uxtl     v5.8h, v1.8b                          // v5 = G widened
    uxtl     v6.8h, v2.8b                          // v6 = B widened
    uxtl     v7.8h, v3.8b                          // v7 = alpha widened (kept, not scaled)

    mul      v4.8h, v4.8h, v7.8h                   // v4 = R * alpha
    mul      v5.8h, v5.8h, v7.8h                   // v5 = G * alpha
    mul      v6.8h, v6.8h, v7.8h                   // v6 = B * alpha

    add      v4.8h, v4.8h, v16.8h                  // v4 += 127
    add      v5.8h, v5.8h, v16.8h                  // v5 += 127
    add      v6.8h, v6.8h, v16.8h                  // v6 += 127

    // ---- Exact divide by 255: (t + (t >> 8)) >> 8, per lane.
    ushr     v17.8h, v4.8h, #8                     // v17 = t >> 8
    add      v4.8h, v4.8h, v17.8h                  // v4  = t + (t >> 8)
    ushr     v4.8h, v4.8h, #8                      // v4  = exact t / 255
    ushr     v17.8h, v5.8h, #8
    add      v5.8h, v5.8h, v17.8h
    ushr     v5.8h, v5.8h, #8
    ushr     v17.8h, v6.8h, #8
    add      v6.8h, v6.8h, v17.8h
    ushr     v6.8h, v6.8h, #8

    // ---- Narrow back to bytes. V3 still holds the ORIGINAL alpha plane and
    // ---- is stored unchanged, which is what keeps alpha unpremultiplied.
    xtn      v0.8b, v4.8h                          // v0 = R narrowed
    xtn      v1.8b, v5.8h                          // v1 = G narrowed
    xtn      v2.8b, v6.8h                          // v2 = B narrowed

    st4      {v0.8b, v1.8b, v2.8b, v3.8b}, [x0], #32   // re-interleave and store 8 pixels

    subs     x9, x9, #8                            // 8 pixels done
    b.ne     .Lloop

.Ldone:
    mov      x0, x3                                // return floor(pixels / 8) * 8
    ret

#ifdef NF_ELF
    NF_ELF_FUNCEND(neoflux_premultiply_rgba8)
#endif
