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

}  // extern "C"
