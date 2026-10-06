// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - asm/abi_macros.inc
//
// Shared assembler macros for the hand-written kernels in this directory.
//
// Included from a .S file (uppercase extension => the C preprocessor runs
// first), so #if/#define are available.
//
// Why this file exists
// --------------------
// Every kernel has to answer the same three portability questions, and getting
// one of them wrong is how a kernel that passes on Linux corrupts the caller on
// Windows:
//
//   1. Does the target C ABI prefix C symbols? Mach-O (macOS/iOS) prepends an
//      underscore; ELF and PE/COFF do not. SYMBOL_NAME hides that.
//   2. Which registers carry the first, second, third integer argument?
//      SysV AMD64 uses RDI/RSI/RDX, Microsoft x64 uses RCX/RDX/R8. The NF_*
//      macros name them so one instruction stream serves both ABIs.
//   3. What has to be written for the assembler to accept, but rejected by
//      another assembler? `.type`/`.size`/`.section .note.GNU-stack` with
//      @progbits are ELF-only; Mach-O and COFF reject all three. NF_ELF_ONLY
//      and NF_ELF_ENDSYM keep that difference in one place.
//
// A routine that takes no integer arguments does not need the NF_* macros;
// xgetbv.S and the prefetch helpers use only SYMBOL_NAME and NF_ELF_*.
// =============================================================================

#if defined(__APPLE__) && defined(__MACH__)
// Mach-O prepends an underscore to every C symbol.
#define SYMBOL_NAME(x) _##x
#define NF_MACHO 1
#else
// ELF (Linux/BSD/Android) and PE/COFF (Windows) do not mangle the leading name.
#define SYMBOL_NAME(x) x
#endif

// Integer argument registers: first = dst, second = src, third = count.
// Only meaningful for x86-64; the AArch64 kernels use x0..x2 directly, which is
// the same register assignment under both AAPCS64 and Win64-ARM.
#if defined(_WIN64)
#define NF_DST %rcx
#define NF_SRC %rdx
#define NF_CNT %r8
#else
#define NF_DST %rdi
#define NF_SRC %rsi
#define NF_CNT %rdx
#endif

// Symbol metadata and the non-executable-stack note are ELF-only. Mach-O and
// COFF assemblers reject .type/@function, .size and @progbits outright.
#if defined(__ELF__)
#define NF_ELF 1
// Opens the function's metadata (must follow a .globl for the symbol).
#define NF_ELF_FUNCBEGIN(name) .type SYMBOL_NAME(name), @function
// Closes it with the symbol size and the non-executable-stack note, then
// restores .text. Without that restoration, a second function in the same
// file is emitted into the discarded GNU-stack section and becomes unlinkable.
#define NF_ELF_FUNCEND(name)                            \
  .size SYMBOL_NAME(name), .-SYMBOL_NAME(name);         \
  .section .note.GNU-stack, "", @progbits;              \
  .text
#endif
