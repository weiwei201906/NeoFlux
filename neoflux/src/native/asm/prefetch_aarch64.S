// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - asm/prefetch_aarch64.S
//
// Cache prefetch hints behind the project's C ABI, for AArch64.
//
// Contract
// --------
//   void neoflux_prefetch_read(const void* p);
//   void neoflux_prefetch_write(const void* p);
//
//   Both are hints. PRFM is architecturally a hint: it does not fault, does not
//   read memory in the architectural sense, and may be ignored entirely by the
//   implementation. `p` may be null or unmapped.
//
// Why assembly instead of __builtin_prefetch
// ------------------------------------------
// The portable builtin is a compiler extension and is not available in the same
// spelling on every toolchain the project builds with. Spelling the instruction
// once keeps the call sites free of compiler-specific knowledge, and lets the
// two entry points differ where the architecture allows it.
//
// PRFM operation selection
// ------------------------
//   Read:  PSTL1KEEP  - prefetch into L1 and mark the line for retention, which
//                       matches the read-mostly render assets this is used on.
//   Write: PSTL1STRM  - streaming hint for lines that will be written and are
//                       not expected to be reused immediately, which avoids
//                       evicting useful lines from L1.
//   Neither form requires a runtime feature probe: PRFM without an offset
//   addressing mode (the `prfm <op>, [xN]` form used here) is a base AArch64
//   instruction, not part of an optional extension.
//
// ABI notes
// ---------
//   - The single pointer argument arrives in x0 under both AAPCS64 and
//     Win64-ARM, so no argument normalization is needed.
//   - No callee-saved register is touched (x0 is caller-saved in both ABIs) and
//     no stack is used.
//   - No vector register is touched, so the AAPCS64 rule that the low 64 bits of
//     v8-v15 must be preserved holds trivially.
// =============================================================================

#include "abi_macros.inc"

    .text

// -----------------------------------------------------------------------------
// void neoflux_prefetch_read(const void* p)
// -----------------------------------------------------------------------------
    .globl SYMBOL_NAME(neoflux_prefetch_read)
    .p2align 2                                     // 4-byte align (instruction alignment)
#ifdef NF_ELF
    NF_ELF_FUNCBEGIN(neoflux_prefetch_read)
#endif
SYMBOL_NAME(neoflux_prefetch_read):
    prfm     pstl1keep, [x0]                       // x0 = p; prefetch into L1, keep the line
    ret
#ifdef NF_ELF
    NF_ELF_FUNCEND(neoflux_prefetch_read)
#endif

// -----------------------------------------------------------------------------
// void neoflux_prefetch_write(const void* p)
// -----------------------------------------------------------------------------
    .globl SYMBOL_NAME(neoflux_prefetch_write)
    .p2align 2
#ifdef NF_ELF
    NF_ELF_FUNCBEGIN(neoflux_prefetch_write)
#endif
SYMBOL_NAME(neoflux_prefetch_write):
    prfm     pstl1strm, [x0]                       // x0 = p; streaming hint for a line about to be written
    ret
#ifdef NF_ELF
    NF_ELF_FUNCEND(neoflux_prefetch_write)
#endif
