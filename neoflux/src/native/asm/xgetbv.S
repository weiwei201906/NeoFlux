// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - asm/xgetbv.S
//
// Contract
// --------
//   unsigned long long neoflux_read_xcr0(void);
//
//   Reads the extended control register XCR0 (the OS-enabled extended
//   register state bitmap) by executing XGETBV with ECX = 0. The 64-bit
//   result is EDX:EAX, i.e. (EDX << 32) | EAX.
//
//   XCR0 bit 0 = x87 state, bit 1 = SSE/XMM state, bit 2 = AVX/YMM state.
//   The tuning layer calls this to confirm the OS actually saves/restores
//   YMM state (XCR0 bits [2:1] == 0b11) before advertising AVX2.
//
// ABI notes
// ---------
//   - The function takes NO arguments. In BOTH System V AMD64 (Linux, macOS,
//     *BSD, Solaris) and Microsoft x64 (Win64) the return value is delivered
//     in RAX/RDX regardless of the integer-argument register assignment, so
//     a single body is correct on every 64-bit x86 ABI. (32-bit cdecl uses
//     EDX:EAX instead; see "64-bit only" below.)
//   - XGETBV only defines EDX:EAX. Bits [63:32] of RAX and all of RDX are
//     architecturally UNSPECIFIED, so we must (a) zero-extend EAX into RAX
//     and (b) shift RDX left by 32 before OR-ing it in, to build the full
//     64-bit value with defined high bits.
//
// 64-bit only
// -----------
//   This file is x86-64 only. i386 is not part of the build matrix; the
//   CMake side selects this source by processor match (AMD64/x86_64), so the
//   assembler never sees a 32-bit target here.
//
// Apple symbol mangling
// ---------------------
//   On Mach-O (macOS) the C symbol `neoflux_read_xcr0` is emitted with a
//   leading underscore, i.e. `_neoflux_read_xcr0`. SYMBOL_NAME below hides
//   that difference so the instruction body stays readable.
// =============================================================================

#if defined(__APPLE__) && defined(__MACH__)
// Mach-O prepends an underscore to every C symbol.
#define SYMBOL_NAME(x) _##x
#else
// ELF (Linux/BSD) and PE/COFF (Windows) do not mangle the leading name.
#define SYMBOL_NAME(x) x
#endif

    .text

// -----------------------------------------------------------------------------
// unsigned long long neoflux_read_xcr0(void)
// -----------------------------------------------------------------------------
    .globl SYMBOL_NAME(neoflux_read_xcr0)
    .p2align 4                                     // 16-byte align: clean decode
#if defined(__ELF__)
    .type  SYMBOL_NAME(neoflux_read_xcr0), @function
#endif
SYMBOL_NAME(neoflux_read_xcr0):
    movl    $0, %ecx                               // ECX = 0 selects XCR0
    xgetbv                                         // EDX:EAX = XCR0 (low 64 bits)
    movl    %eax, %eax                             // zero-extend EAX -> RAX ([63:32]=0)
    shlq    $32, %rdx                              // move XCR0[63:32] into RDX[63:32]
    orq     %rdx, %rax                             // assemble (EDX << 32) | EAX
    ret                                            // return in RAX (SysV & Win64)

// Symbol metadata and the non-executable-stack note are ELF-only: Mach-O and
// COFF assemblers reject .type/@function, .size and @progbits outright.
#if defined(__ELF__)
    .size   SYMBOL_NAME(neoflux_read_xcr0), .-SYMBOL_NAME(neoflux_read_xcr0)
    .section .note.GNU-stack, "", @progbits
#endif
