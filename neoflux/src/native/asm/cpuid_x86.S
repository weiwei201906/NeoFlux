// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - asm/cpuid_x86.S
//
// CPUID feature queries behind the project's C ABI.
//
// Contract
// --------
//   void neoflux_cpuid(std::uint32_t leaf, std::uint32_t* regs);
//     CPUID leaf, subleaf 0. Writes EAX, EBX, ECX, EDX to regs[0..3].
//
//   void neoflux_cpuid_subleaf(std::uint32_t leaf, std::uint32_t subleaf,
//                              std::uint32_t* regs);
//     CPUID leaf, explicit subleaf (needed for leaf 4 and leaf 7).
//
//   Both return void and have no failure mode. `regs` must point at four
//   std::uint32_t and is owned by the caller. An unsupported leaf returns the
//   highest-leaf data or zeros, which the callers already handle: leaf 4's
//   cache-type field of 0 terminates the cache walk, and an unknown feature bit
//   reads as "not present".
//
// Why assembly instead of the compiler intrinsic
// ----------------------------------------------
// The intrinsic is spelled differently on every toolchain: __cpuidex from
// <intrin.h> on MSVC and MinGW, __get_cpuid_count from <cpuid.h> on GCC and
// Clang. Using them means the feature-detection code has to exist twice, and
// the two copies drift - which is how "the fix for Linux breaks Windows"
// happens. One instruction sequence behind one C ABI removes the difference
// instead of managing it.
//
// ABI notes (why this file is safe on both 64-bit x86 ABIs)
// ---------------------------------------------------------
//   - RBX is callee-saved on BOTH Microsoft x64 and SysV AMD64, and CPUID
//     overwrites it. It is pushed on entry and popped before the return, so the
//     caller's RBX is preserved without the caller knowing CPUID was used.
//   - The argument registers differ between the ABIs, so the entry block moves
//     the incoming arguments into one canonical set and the body never has to
//     care which ABI it is running under.
//   - R9/R10/R11 are caller-saved on both ABIs and are used as scratch.
//   - No vector register is touched, so the Win64 rule that xmm6-xmm15 must be
//     preserved holds trivially.
//   - No stack space beyond the saved RBX is used and no red zone is relied on,
//     so the same code is correct on Windows, where no red zone exists.
//
// 64-bit only
// -----------
//   This file is x86-64 only. CMake selects it by processor match, so the
//   assembler never sees a 32-bit target. A 32-bit build would need the
//   stack-based operand form of CPUID instead of the register one.
// =============================================================================

#include "abi_macros.inc"

    .text

// -----------------------------------------------------------------------------
// void neoflux_cpuid(uint32_t leaf, uint32_t* regs)
//
//   SysV:  leaf = edi, regs = rsi
//   Win64: leaf = ecx, regs = rdx
// -----------------------------------------------------------------------------
    .globl SYMBOL_NAME(neoflux_cpuid)
    .p2align 4
#ifdef NF_ELF
    NF_ELF_FUNCBEGIN(neoflux_cpuid)
#endif
SYMBOL_NAME(neoflux_cpuid):
    pushq    %rbx                                  // RBX is callee-saved and CPUID clobbers it
    movq     %rsi, %r9                             // R9 = regs (SysV)
    movl     %edi, %eax                            // EAX = leaf (SysV)
#if defined(_WIN64)
    movq     %rdx, %r9                             // R9 = regs (Win64)
    movl     %ecx, %eax                            // EAX = leaf (Win64)
#endif
    xorl     %ecx, %ecx                            // ECX = 0 -> subleaf 0
    cpuid                                          // EAX/EBX/ECX/EDX = leaf data
    movl     %eax, 0(%r9)                          // regs[0] = EAX
    movl     %ebx, 4(%r9)                          // regs[1] = EBX
    movl     %ecx, 8(%r9)                          // regs[2] = ECX
    movl     %edx, 12(%r9)                         // regs[3] = EDX
    popq     %rbx                                  // restore the caller's RBX
    ret
#ifdef NF_ELF
    NF_ELF_FUNCEND(neoflux_cpuid)
#endif

// -----------------------------------------------------------------------------
// void neoflux_cpuid_subleaf(uint32_t leaf, uint32_t subleaf, uint32_t* regs)
//
//   SysV:  leaf = edi, subleaf = esi, regs = rdx
//   Win64: leaf = ecx, subleaf = edx, regs = r8
// -----------------------------------------------------------------------------
    .globl SYMBOL_NAME(neoflux_cpuid_subleaf)
    .p2align 4
#ifdef NF_ELF
    NF_ELF_FUNCBEGIN(neoflux_cpuid_subleaf)
#endif
SYMBOL_NAME(neoflux_cpuid_subleaf):
    pushq    %rbx                                  // RBX is callee-saved and CPUID clobbers it
    movq     %rdx, %r9                             // R9 = regs (SysV)
    movl     %edi, %eax                            // EAX = leaf (SysV)
    movl     %esi, %r10d                           // R10D = subleaf (SysV; R10 survives until ECX is set)
#if defined(_WIN64)
    movq     %r8, %r9                              // R9 = regs (Win64)
    movl     %ecx, %eax                            // EAX = leaf (Win64)
    movl     %edx, %r10d                           // R10D = subleaf (Win64)
#endif
    movl     %r10d, %ecx                           // ECX = subleaf
    cpuid                                          // EAX/EBX/ECX/EDX = leaf data
    movl     %eax, 0(%r9)                          // regs[0] = EAX
    movl     %ebx, 4(%r9)                          // regs[1] = EBX
    movl     %ecx, 8(%r9)                          // regs[2] = ECX
    movl     %edx, 12(%r9)                         // regs[3] = EDX
    popq     %rbx                                  // restore the caller's RBX
    ret
#ifdef NF_ELF
    NF_ELF_FUNCEND(neoflux_cpuid_subleaf)
#endif
