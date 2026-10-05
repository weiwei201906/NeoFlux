// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - native/simd_kernels.cpp
//
// Dispatch layer for the hand-written kernels in src/native/asm.
//
// Nothing here includes an intrinsic header. The only channel to the assembly
// is the plain C ABI declared in native/asm/asm_symbols.h, which is what makes
// the kernels independent of any compiler's inline-asm or intrinsic model:
// GCC, Clang, MSVC and AppleClang all just see an external symbol.
//
// Selection is compile time and comes from CMake: NEOFLUX_NATIVE_ASM_
// PREMULTIPLY_X86_64 / _AARCH64 are defined only when the matching .S file
// was actually added to the target, so these constants cannot disagree with
// what was linked. Builds where no assembly was compiled in (MSVC, which needs
// MASM rather than GAS syntax, or a toolchain without an assembler) fall back
// to the scalar kernel, which is always compiled in and is also what finishes
// the tail.
// =============================================================================

#include "native/simd_kernels.h"

#include "native/asm/asm_symbols.h"

namespace neoflux {
namespace native {
namespace {

// Pixels consumed per assembly pass. Each constant is guarded by the macro
// that selects its kernel rather than being defined unconditionally: an
// unused constexpr triggers -Wunused-const-variable, and the build compiles
// with -Werror, so declaring the other architecture's group size here would
// fail every build that does not use it.
#if defined(NEOFLUX_NATIVE_ASM_PREMULTIPLY_X86_64)
// One 16-byte SSE2 register holds exactly four RGBA pixels.
constexpr std::size_t kGroupX86_64 = 4;
#endif
#if defined(NEOFLUX_NATIVE_ASM_PREMULTIPLY_AARCH64)
// LD4/ST4 move four 8-byte channel planes: eight pixels per pass.
constexpr std::size_t kGroupAarch64 = 8;
#endif

// RGBA8 layout: four 8-bit channels per pixel, alpha in the last one. Used
// unguarded because the scalar kernel below is always compiled in.
constexpr std::size_t kChannelsPerPixel = 4;

// Exact division by 255 with the round-half-up term 127:
//   t = x + 127;  result = (t + (t >> 8)) >> 8
// Identical to what both assembly kernels compute, so all three paths are
// bit-identical by construction rather than by coincidence.
constexpr std::uint8_t PremultiplyChannel(std::uint8_t channel,
                                          std::uint8_t alpha) noexcept {
  const std::uint32_t product =
      static_cast<std::uint32_t>(channel) * static_cast<std::uint32_t>(alpha);
  const std::uint32_t rounded = product + 127U;
  return static_cast<std::uint8_t>((rounded + (rounded >> 8)) >> 8);
}

// Portable kernel. Always compiled in: it serves both as the tail finisher
// and as the entire implementation on targets without a shipped assembly
// unit. Deliberately dumb, one byte at a time, so it can act as the reference
// the assembly is checked against.
void PremultiplyScalar(std::uint8_t* dst, const std::uint8_t* src,
                       std::size_t pixels) noexcept {
  for (std::size_t i = 0; i < pixels; ++i) {
    const std::size_t base = i * kChannelsPerPixel;
    const std::uint8_t alpha = src[base + 3];
    dst[base + 0] = PremultiplyChannel(src[base + 0], alpha);
    dst[base + 1] = PremultiplyChannel(src[base + 1], alpha);
    dst[base + 2] = PremultiplyChannel(src[base + 2], alpha);
    dst[base + 3] = alpha;
  }
}

// Compile-time dispatch. Derived from the same CMake macros that decide which
// .S file is added to the target, so there is no way for this to claim a
// kernel that was not linked.
#if defined(NEOFLUX_NATIVE_ASM_PREMULTIPLY_X86_64)
constexpr SimdLevel kActiveLevel = SimdLevel::kSse2;
constexpr std::size_t kActiveGroup = kGroupX86_64;
#elif defined(NEOFLUX_NATIVE_ASM_PREMULTIPLY_AARCH64)
constexpr SimdLevel kActiveLevel = SimdLevel::kNeon;
constexpr std::size_t kActiveGroup = kGroupAarch64;
#else
constexpr SimdLevel kActiveLevel = SimdLevel::kScalar;
constexpr std::size_t kActiveGroup = 0;
#endif

}  // namespace

void PremultiplyRgba8(std::uint8_t* dst, const std::uint8_t* src,
                      std::size_t pixels) {
  if (dst == nullptr || src == nullptr || pixels == 0) {
    return;
  }

  std::size_t done = 0;
  // Guarded by the preprocessor, NOT by `if (kActiveGroup != 0)`: a constant
  // false runtime condition still leaves a link-time reference to the
  // assembly symbol in builds that compile none, and relying on dead-code
  // elimination to erase that reference is not something any compiler
  // promises. This is the difference between "skipped" and "failed to link".
#if defined(NEOFLUX_NATIVE_ASM_PREMULTIPLY)
  done = neoflux_premultiply_rgba8(dst, src, pixels);
#endif
  if (done < pixels) {
    PremultiplyScalar(dst + done * 4, src + done * 4, pixels - done);
  }
}

SimdLevel ActiveSimdLevel() noexcept { return kActiveLevel; }

std::size_t SimdGroupPixels() noexcept { return kActiveGroup; }

}  // namespace native
}  // namespace neoflux
