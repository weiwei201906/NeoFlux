// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - native/cpuid_bits.h
//
// The only place in the framework that knows how to read a CPUID register.
//
// Why it exists
// -------------
// CPUID is spelled differently on every toolchain: __cpuidex from <intrin.h> on
// MSVC and MinGW, __get_cpuid_count from <cpuid.h> on GCC and Clang. Writing the
// feature detection once per toolchain is how the copies drift apart, and a
// drifted copy is how a change verified on Linux breaks Windows.
//
// The portable routes are, in order of preference:
//
//   1. NEOFLUX_NATIVE_ASM_CPUID  CMake defined it, so asm/cpuid_x86.S is linked
//      and neoflux_cpuid_subleaf() is available. One instruction sequence, one
//      ABI, identical behaviour on Linux, macOS, MinGW and clang-cl.
//   2. __cpuidex                 MSVC, and clang-cl: both provide it through
//      <intrin.h>. This is the one compiler intrinsic the project still needs,
//      and it is confined to this file.
//   3. Unavailable               The header still compiles and reports zeros;
//      callers must then treat every optional feature as absent. That is the
//      correct default: advertising a feature we could not probe is worse than
//      reporting it missing.
//
// A CPUID register is architecturally 32 bits wide, which is why everything
// here is std::uint32_t rather than int. The intrinsic takes unsigned int*,
// so the only concession is the local array below.
// =============================================================================

#pragma once

#include <array>
#include <cstdint>

#include "native/asm/asm_symbols.h"

#if !defined(NEOFLUX_NATIVE_ASM_CPUID) && defined(_MSC_VER)
// MSVC (and clang-cl, which defines _MSC_VER) provide __cpuidex here. This is
// the single intrinsic header the framework includes, and it is included from
// the dispatch layer only - business and pipeline code never see it.
#include <intrin.h>
#define NEOFLUX_HAVE_CPUIDEX 1
#endif

namespace neoflux::native::cpuid_bits {

/// True when some route to a real CPUID read exists in this build.
[[nodiscard]] constexpr bool Available() noexcept {
#if defined(NEOFLUX_NATIVE_ASM_CPUID) || defined(NEOFLUX_HAVE_CPUIDEX)
  return true;
#else
  return false;
#endif
}

/// Runs CPUID for `leaf` and `subleaf`, returning EAX, EBX, ECX, EDX in that
/// order. When no route is available every field is zero, which callers read as
/// "feature absent" and "no further topology entries".
[[nodiscard]] inline std::array<std::uint32_t, 4> Subleaf(
    std::uint32_t leaf, std::uint32_t subleaf) noexcept {
  std::array<std::uint32_t, 4> out{0U, 0U, 0U, 0U};
#if defined(NEOFLUX_NATIVE_ASM_CPUID)
  neoflux_cpuid_subleaf(leaf, subleaf, out.data());
#elif defined(NEOFLUX_HAVE_CPUIDEX)
  // __cpuidex takes int[4] and int operands. The values are bit patterns, so
  // the signed round trip loses nothing; the results are read back as unsigned.
  int regs[4] = {0, 0, 0, 0};
  __cpuidex(regs, static_cast<int>(leaf), static_cast<int>(subleaf));
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = static_cast<std::uint32_t>(regs[i]);
  }
#else
  (void)leaf;
  (void)subleaf;
#endif
  return out;
}

}  // namespace neoflux::native::cpuid_bits
