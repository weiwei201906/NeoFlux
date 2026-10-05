// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - asm/premultiply_rgba_x86_64.S
//
// Contract
// --------
//   size_t neoflux_premultiply_rgba8(uint8_t* dst, const uint8_t* src,
//                                    size_t pixels);
//
//   Converts |pixels| straight (non-premultiplied) RGBA8 pixels from |src|
//   into premultiplied RGBA8 in |dst|, four channels per pixel, in place-safe
//   (|dst| may alias |src|):
//
//     dst[c] = (src[c] * src[a] + 127) / 255    for c in {R, G, B}
//     dst[a] = src[a]
//
//   Returns the number of pixels actually converted: floor(pixels / 4) * 4.
//   Pixels beyond that count are NOT touched. The caller (the dispatch layer
//   in src/native/simd_kernels.cpp) finishes the remaining 0..3 pixels with
//   the portable scalar kernel, which computes the same expression.
//
//   The division by 255 is exact, not approximate. For |x| = channel * alpha
//   (max 65025) the rounding term makes |t| = x + 127 (max 65152), and
//
//     (t + (t >> 8)) >> 8
//
//   equals floor(t / 255) for every t in [0, 65152]. Intermediate values stay
//   below 65406, so no 16-bit lane ever overflows and no widening to 32 bits
//   is needed. Do not "simplify" this to a shift by 8 alone: that is off by
//   one for every value the rounding term pushes across a boundary.
//
// Instruction set
// ---------------
//   SSE2 only. SSE2 is an architectural baseline of x86-64 (every x86-64
//   implementation guarantees it), so there is no CPUID probe here and none
//   is needed: the dispatch layer picks this file purely by target
//   architecture. A future AVX2 kernel would NOT be a baseline and would have
//   to be gated on CPUID + XCR0 at runtime (see asm/xgetbv.S).
//
// ABI notes (why this file is safe on both 64-bit x86 ABIs)
// ---------------------------------------------------------
//   - Integer arguments: System V AMD64 uses RDI/RSI/RDX; Microsoft x64 uses
//     RCX/RDX/R8. NF_DST / NF_SRC / NF_CNT below hide that difference so a
//     single instruction stream serves both.
//   - Vector registers: Win64 makes XMM6-XMM15 callee-saved, while SysV makes
//     every XMM caller-saved. The intersection is XMM0-XMM5, so this kernel
//     restricts itself to exactly those six. Touching XMM6 or above would
//     silently corrupt the caller's registers on Windows only, which is the
//     classic way a hand-written kernel passes on Linux and fails on Windows.
//   - R10 is caller-saved on both ABIs and is used as the loop counter. RAX
//     holds the return value. No callee-saved register is touched, so there
//     is nothing to save and restore.
//
// 64-bit only
// -----------
//   x86-64 only. i386 is not part of the build matrix; CMake selects this
//   source by processor match, so the assembler never sees a 32-bit target.
//
// Apple symbol mangling
// ---------------------
//   On Mach-O (macOS) the C symbol `neoflux_premultiply_rgba8` is emitted
//   with a leading underscore. SYMBOL_NAME below hides that difference so the
//   instruction body stays readable.
// =============================================================================

#if defined(__APPLE__) && defined(__MACH__)
// Mach-O prepends an underscore to every C symbol.
#define SYMBOL_NAME(x) _##x
#else
// ELF (Linux/BSD) and PE/COFF (Windows) do not mangle the leading name.
#define SYMBOL_NAME(x) x
#endif

// Integer argument registers: first = dst, second = src, third = pixel count.
#if defined(_WIN64)
#define NF_DST %rcx
#define NF_SRC %rdx
#define NF_CNT %r8
#else
#define NF_DST %rdi
#define NF_SRC %rsi
#define NF_CNT %rdx
#endif

    .text

// -----------------------------------------------------------------------------
// size_t neoflux_premultiply_rgba8(uint8_t* dst, const uint8_t* src,
//                                  size_t pixels)
// -----------------------------------------------------------------------------
    .globl SYMBOL_NAME(neoflux_premultiply_rgba8)
    .p2align 4                                     // 16-byte align: clean decode
    .type  SYMBOL_NAME(neoflux_premultiply_rgba8), @function
SYMBOL_NAME(neoflux_premultiply_rgba8):
    // ---- Constants, built with ALU ops so the file stays position
    // ---- independent and needs no .rodata section.
    pxor     %xmm0, %xmm0                          // xmm0 = all zero (byte -> word widening)
    pcmpeqd  %xmm1, %xmm1                          // xmm1 = all ones (every word = 0xFFFF)
    psrlw    $9, %xmm1                             // 0xFFFF >> 9 = 0x007F -> xmm1 = 127 (rounding term)

    // ---- Return value: floor(pixels / 4) * 4, kept in RAX for the whole call.
    movq     NF_CNT, %rax                          // rax = requested pixel count
    andq     $-4, %rax                             // mask off the 0..3 pixel tail
    movq     %rax, %r10                            // r10 = pixels still to convert

    testq    %r10, %r10                            // count < 4: nothing to do
    je       .Ldone

.Lloop:
    movdqu   (NF_SRC), %xmm3                       // xmm3 = 4 pixels = 16 bytes (unaligned load)
    movdqa   %xmm3, %xmm4                          // keep a copy: the low/high halves are split
    punpcklbw %xmm0, %xmm3                         // xmm3 = pixels 0..1, each byte widened to a word
    punpckhbw %xmm0, %xmm4                         // xmm4 = pixels 2..3, likewise

    // ---- Low half (xmm3 holds [R0 G0 B0 A0 R1 G1 B1 A1] as words).
    movdqa   %xmm3, %xmm5                          // xmm5 = the channels, saved
    pshuflw  $0xFF, %xmm3, %xmm3                   // low 4 words := word 3  -> A0 broadcast
    pshufhw  $0xFF, %xmm3, %xmm3                   // high 4 words := word 7 -> A1 broadcast
    pmullw   %xmm3, %xmm5                          // xmm5 = channel * alpha (max 65025, fits u16)
    paddw    %xmm1, %xmm5                          // xmm5 += 127 (rounding term)
    movdqa   %xmm5, %xmm2                          // xmm2 = t
    psrlw    $8, %xmm2                             // xmm2 = t >> 8
    paddw    %xmm2, %xmm5                          // xmm5 = t + (t >> 8)   (max 65406, no overflow)
    psrlw    $8, %xmm5                             // xmm5 = exact t / 255; lanes 3 and 7 are junk
    pcmpeqd  %xmm2, %xmm2                          // rebuild the alpha mask (2 ops, cheaper than
    psllq    $48, %xmm2                            //   holding it): lanes 3 and 7 = 0xFFFF
    pand     %xmm2, %xmm3                          // xmm3 = [0 0 0 A0 0 0 0 A1]
    pandn    %xmm5, %xmm2                          // xmm2 = ~mask & result -> RGB kept, alpha cleared
    por      %xmm3, %xmm2                          // xmm2 = low half, alpha restored

    // ---- High half, identical numerics on xmm4.
    movdqa   %xmm4, %xmm5                          // xmm5 = the channels, saved
    pshuflw  $0xFF, %xmm4, %xmm4                   // A2 broadcast into words 0..3
    pshufhw  $0xFF, %xmm4, %xmm4                   // A3 broadcast into words 4..7
    pmullw   %xmm4, %xmm5                          // xmm5 = channel * alpha
    paddw    %xmm1, %xmm5                          // xmm5 += 127
    movdqa   %xmm5, %xmm3                          // xmm3 = t
    psrlw    $8, %xmm3                             // xmm3 = t >> 8
    paddw    %xmm3, %xmm5                          // xmm5 = t + (t >> 8)
    psrlw    $8, %xmm5                             // xmm5 = exact t / 255
    pcmpeqd  %xmm3, %xmm3                          // rebuild the alpha mask
    psllq    $48, %xmm3                            // lanes 3 and 7 = 0xFFFF
    pand     %xmm3, %xmm4                          // xmm4 = [0 0 0 A2 0 0 0 A3]
    pandn    %xmm5, %xmm3                          // xmm3 = ~mask & result
    por      %xmm4, %xmm3                          // xmm3 = high half, alpha restored

    packuswb %xmm3, %xmm2                          // xmm2 = 16 result bytes: low half, then high half
    movdqu   %xmm2, (NF_DST)                       // store 4 premultiplied pixels (unaligned)

    addq     $16, NF_SRC                           // advance source by 4 pixels
    addq     $16, NF_DST                           // advance destination by 4 pixels
    subq     $4, %r10                              // 4 pixels done
    jne      .Lloop

.Ldone:
    ret                                            // rax = floor(pixels / 4) * 4
    .size   SYMBOL_NAME(neoflux_premultiply_rgba8), .-SYMBOL_NAME(neoflux_premultiply_rgba8)

// Mark the stack non-executable (ELF toolchains). Harmless elsewhere.
#if !defined(__APPLE__)
    .section .note.GNU-stack, "", @progbits
#endif
