// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - asm/prefetch_x86.S
//
// Cache prefetch hints behind the project's C ABI.
//
// Contract
// --------
//   void neoflux_prefetch_read(const void* p);
//   void neoflux_prefetch_write(const void* p);
//
//   Both are hints: they cannot fault and they do not read memory
//   architecturally, so passing an address that is not mapped, or the same
//   address twice, is well defined. `p` may be null.
//
// Why assembly instead of __builtin_prefetch / _mm_prefetch
// ---------------------------------------------------------
// The portable builtin is a compiler extension, and the two MSVC forms
// (_mm_prefetch with _MM_HINT_T0) sit behind an intrinsic header that the
// project's policy forbids outside this directory. Spelling the instruction
// once keeps the call sites free of any compiler-specific knowledge.
//
// PREFETCHT0 is the strongest temporal hint (prefetch into all cache levels)
// and is an SSE instruction, so no runtime CPU feature probe is needed on any
// x86-64 target. The write form deliberately uses the same PREFETCHT0 rather
// than PREFETCHW: PREFETCHW is an AMD extension that is not part of the
// x86-64 baseline, and a hint is not worth a feature gate. The distinction
// between the two entry points is kept in the API so that a future target that
// does provide a write-allocate hint can use it without changing callers.
//
// ABI notes
// ---------
//   - The single pointer argument arrives in RDI (SysV) or RCX (Win64); the
//     entry block normalizes it into RAX, which is caller-saved on both ABIs.
//   - No callee-saved register is touched and no stack is used, so there is
//     nothing to preserve on either ABI.
//   - No vector register is touched.
// =============================================================================

#include "abi_macros.inc"

    .text

// -----------------------------------------------------------------------------
// void neoflux_prefetch_read(const void* p)
// -----------------------------------------------------------------------------
    .globl SYMBOL_NAME(neoflux_prefetch_read)
    .p2align 4
#ifdef NF_ELF
    NF_ELF_FUNCBEGIN(neoflux_prefetch_read)
#endif
SYMBOL_NAME(neoflux_prefetch_read):
    movq     %rdi, %rax                            // RAX = p (SysV)
#if defined(_WIN64)
    movq     %rcx, %rax                            // RAX = p (Win64)
#endif
    prefetcht0 (%rax)                              // bring the line into all cache levels
    ret
#ifdef NF_ELF
    NF_ELF_FUNCEND(neoflux_prefetch_read)
#endif

// -----------------------------------------------------------------------------
// void neoflux_prefetch_write(const void* p)
// -----------------------------------------------------------------------------
    .globl SYMBOL_NAME(neoflux_prefetch_write)
    .p2align 4
#ifdef NF_ELF
    NF_ELF_FUNCBEGIN(neoflux_prefetch_write)
#endif
SYMBOL_NAME(neoflux_prefetch_write):
    movq     %rdi, %rax                            // RAX = p (SysV)
#if defined(_WIN64)
    movq     %rcx, %rax                            // RAX = p (Win64)
#endif
    prefetcht0 (%rax)                              // same hint as the read form; see the header
    ret
#ifdef NF_ELF
    NF_ELF_FUNCEND(neoflux_prefetch_write)
#endif
