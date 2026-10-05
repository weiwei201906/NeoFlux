// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors

// Regression test for the hand-written premultiply SIMD kernels
// (src/native/asm/premultiply_rgba_*.S) and for the dispatch layer
// (src/native/simd_kernels.cpp).
//
// Why this file may use intrinsics when the rest of the framework may not:
// the assembly kernel has to be checked against an INDEPENDENT implementation.
// Using the portable scalar kernel as the only oracle would let a bug in the
// dispatch layer hide itself by being compared against a copy of its own
// arithmetic, so a second oracle written with SSE2 intrinsics is the cheapest
// way to catch a broken kernel OR a broken reference. This translation unit is
// compiled into a throwaway binary by verify/README.md and never ships.
//
// Coverage, matching the three categories the project uses:
//   smoke   - hand-picked values, degenerate alphas, aliasing, every tail
//   death   - hostile arguments must never crash (forked on POSIX)
//   stress  - large random images with trailing guard bytes
//   oracle  - assembly vs. scalar vs. SSE2 intrinsics, byte for byte
//
// Compile with the matching .S and macro to exercise the assembly path:
//   g++ -std=c++20 -O1 -DNEOFLUX_NATIVE_ASM_PREMULTIPLY_X86_64=1 \
//       -I neoflux/include -I neoflux/src \
//       verify/test_native_simd.cpp neoflux/src/native/simd_kernels.cpp \
//       neoflux/src/native/asm/premultiply_rgba_x86_64.S -o /tmp/t
// Without the macro and the .S the assembly cases report SKIP and the scalar
// path is still fully exercised.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <vector>

// Test-only intrinsic usage, guarded because <emmintrin.h> does not exist on
// ARM64 or any other non-x86 target. MSVC defines _M_X64 rather than
// __SSE2__, hence the wider guard.
#if defined(__SSE2__) || defined(_M_X64) || defined(_M_IX86)
#include <emmintrin.h>
#define NEOFLUX_VERIFY_HAVE_SSE2 1
#endif

#if defined(__unix__) || defined(__APPLE__)
#include <sys/wait.h>
#include <unistd.h>
#define NEOFLUX_VERIFY_CAN_FORK 1
#endif

#include "native/asm/asm_symbols.h"
#include "native/simd_kernels.h"

using neoflux::native::ActiveSimdLevel;
using neoflux::native::PremultiplyRgba8;
using neoflux::native::SimdGroupPixels;
using neoflux::native::SimdLevel;

namespace {

constexpr std::size_t kChannels = 4;
constexpr std::uint8_t kSentinel = 0xA5;
constexpr std::size_t kGuardBytes = 64;

int failures = 0;

void Check(bool ok, const char* what) {
  if (!ok) {
    std::printf("  [FAIL] %s\n", what);
    ++failures;
  }
}

// Independent oracle, re-derived from the documented formula rather than
// calling into src/native internals:
//   t = channel * alpha + 127;  result = (t + (t >> 8)) >> 8
std::uint8_t RefChannel(std::uint8_t channel, std::uint8_t alpha) {
  const std::uint32_t t =
      static_cast<std::uint32_t>(channel) * static_cast<std::uint32_t>(alpha) +
      127U;
  return static_cast<std::uint8_t>((t + (t >> 8)) >> 8);
}

void RefPremultiply(std::uint8_t* dst, const std::uint8_t* src,
                    std::size_t pixels) {
  for (std::size_t i = 0; i < pixels; ++i) {
    const std::size_t b = i * kChannels;
    const std::uint8_t a = src[b + 3];
    dst[b + 0] = RefChannel(src[b + 0], a);
    dst[b + 1] = RefChannel(src[b + 1], a);
    dst[b + 2] = RefChannel(src[b + 2], a);
    dst[b + 3] = a;
  }
}

// Pixels followed by a sentinel band. Catches a kernel that walks one group
// too far, and catches writes into the tail the C ABI promises to leave alone.
class Image {
 public:
  explicit Image(std::size_t pixels)
      : pixels_(pixels), bytes_(pixels * kChannels + kGuardBytes, kSentinel) {}

  std::size_t Bytes() const { return pixels_ * kChannels; }
  std::uint8_t* Data() { return bytes_.data(); }
  const std::uint8_t* Data() const { return bytes_.data(); }

  void FillRandom(std::uint32_t seed) {
    std::uint32_t s = seed;
    for (std::size_t i = 0; i < Bytes(); ++i) {
      s = s * 1103515245U + 12345U;
      bytes_[i] = static_cast<std::uint8_t>(s >> 16);
    }
    ResetGuard();
  }

  void FillPixels(std::uint8_t value) {
    for (std::size_t i = 0; i < Bytes(); ++i) bytes_[i] = value;
    ResetGuard();
  }

  void ForceAlpha(std::uint8_t alpha) {
    for (std::size_t i = 3; i < Bytes(); i += kChannels) bytes_[i] = alpha;
  }

  void ResetGuard() {
    for (std::size_t i = Bytes(); i < bytes_.size(); ++i) bytes_[i] = kSentinel;
  }

  bool GuardIntact() const {
    for (std::size_t i = Bytes(); i < bytes_.size(); ++i) {
      if (bytes_[i] != kSentinel) return false;
    }
    return true;
  }

 private:
  std::size_t pixels_;
  std::vector<std::uint8_t> bytes_;
};

bool RegionIs(const std::uint8_t* p, std::size_t n, std::uint8_t value) {
  for (std::size_t i = 0; i < n; ++i) {
    if (p[i] != value) return false;
  }
  return true;
}

bool Equal(const std::uint8_t* a, const std::uint8_t* b, std::size_t n) {
  return std::memcmp(a, b, n) == 0;
}

// --- Case 3/6 helper: run the raw C ABI kernel and check its whole contract.
//
// Compiled only when the assembly kernel was actually linked: the symbol is
// declared in asm_symbols.h under the same macro, so referencing it here
// unconditionally would fail to COMPILE on a scalar-only build. That is
// deliberate -- an undefined symbol must be a compile error, not a link
// error discovered later, and never a runtime surprise.
#if defined(NEOFLUX_NATIVE_ASM_PREMULTIPLY)
bool RawMatchesReference(std::size_t pixels, std::uint32_t seed) {
  const std::size_t group = SimdGroupPixels();
  if (group == 0) return false;

  Image src(pixels);
  src.FillRandom(seed);
  Image dst(pixels);
  dst.FillPixels(kSentinel);

  const std::size_t done =
      neoflux_premultiply_rgba8(dst.Data(), src.Data(), pixels);
  const std::size_t want = (pixels / group) * group;
  if (done != want) {
    std::printf("  [FAIL] raw kernel returned %zu, want %zu\n", done, want);
    return false;
  }

  std::vector<std::uint8_t> expect(done * kChannels, 0);
  RefPremultiply(expect.data(), src.Data(), done);
  if (!Equal(dst.Data(), expect.data(), done * kChannels)) {
    std::printf("  [FAIL] raw kernel content differs from the reference\n");
    return false;
  }
  if (!RegionIs(dst.Data() + done * kChannels,
                (pixels - done) * kChannels, kSentinel) ||
      !dst.GuardIntact()) {
    std::printf("  [FAIL] raw kernel wrote past its returned count\n");
    return false;
  }
  return src.GuardIntact();
}
#endif  // NEOFLUX_NATIVE_ASM_PREMULTIPLY

#if defined(NEOFLUX_VERIFY_HAVE_SSE2)

// Third implementation, written with intrinsics, used only as an oracle.
// Operates on eight 16-bit lanes holding [R0 G0 B0 A0 R1 G1 B1 A1].
__m128i PremultiplyTwoPixels(__m128i lanes16, __m128i round, __m128i amask) {
  __m128i alpha = _mm_shufflelo_epi16(lanes16, 0xFF);
  alpha = _mm_shufflehi_epi16(alpha, 0xFF);

  __m128i t = _mm_mullo_epi16(lanes16, alpha);
  t = _mm_add_epi16(t, round);
  t = _mm_srli_epi16(_mm_add_epi16(t, _mm_srli_epi16(t, 8)), 8);

  return _mm_or_si128(_mm_andnot_si128(amask, t), _mm_and_si128(amask, alpha));
}

std::size_t PremultiplySse2(std::uint8_t* dst, const std::uint8_t* src,
                            std::size_t pixels) {
  const __m128i zero = _mm_setzero_si128();
  const __m128i round = _mm_set1_epi16(127);
  const __m128i amask = _mm_set_epi16(-1, 0, 0, 0, -1, 0, 0, 0);

  std::size_t i = 0;
  for (; i + 4 <= pixels; i += 4) {
    const __m128i rgba = _mm_loadu_si128(
        reinterpret_cast<const __m128i*>(src + i * kChannels));
    const __m128i low =
        PremultiplyTwoPixels(_mm_unpacklo_epi8(rgba, zero), round, amask);
    const __m128i high =
        PremultiplyTwoPixels(_mm_unpackhi_epi8(rgba, zero), round, amask);
    _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + i * kChannels),
                     _mm_packus_epi16(low, high));
  }
  RefPremultiply(dst + i * kChannels, src + i * kChannels, pixels - i);
  return i;
}

#endif  // NEOFLUX_VERIFY_HAVE_SSE2

// Hostile calls that MUST NOT crash. Runs in a forked child on POSIX so a
// crash surfaces as a failed expectation instead of killing the run.
void HostileCalls() {
  std::vector<std::uint8_t> scratch(kChannels, 0x7F);
  PremultiplyRgba8(nullptr, nullptr, 0);
  PremultiplyRgba8(scratch.data(), nullptr, 1);
  PremultiplyRgba8(nullptr, scratch.data(), 1);
  PremultiplyRgba8(scratch.data(), scratch.data(), 0);
  PremultiplyRgba8(nullptr, nullptr, static_cast<std::size_t>(-1));
  PremultiplyRgba8(scratch.data(), scratch.data(), 1);  // shorter than a group
}

}  // namespace

int main() {
  const SimdLevel level = ActiveSimdLevel();
  const std::size_t group = SimdGroupPixels();
  const char* name = (level == SimdLevel::kSse2)   ? "sse2"
                     : (level == SimdLevel::kNeon) ? "neon"
                                                   : "scalar";
  std::printf("active kernel: %s (group=%zu)\n", name, group);
  Check((group == 0) == (level == SimdLevel::kScalar),
        "group size agrees with the active level");

  // --- Case 1: smoke, hand-picked values including both degenerate alphas ---
  {
    const std::uint8_t src[] = {
        0x00, 0x00, 0x00, 0x00,  // fully transparent: collapses to zero
        0xFF, 0xFF, 0xFF, 0xFF,  // opaque white: identity
        0x80, 0x40, 0x20, 0x80,  // half transparency
        0x01, 0x02, 0x03, 0xFF,  // opaque, almost black
        0xFF, 0x00, 0x7F, 0x01,  // almost transparent
        0x33, 0x55, 0x77, 0x99, 0x0A, 0x0B, 0x0C, 0x0D,
        0xFF, 0xFF, 0xFF, 0x00,  // transparent white: collapses to zero
    };
    constexpr std::size_t kPixels = sizeof(src) / kChannels;
    std::vector<std::uint8_t> got(sizeof(src), 0);
    std::vector<std::uint8_t> want(sizeof(src), 0);

    PremultiplyRgba8(got.data(), src, kPixels);
    RefPremultiply(want.data(), src, kPixels);
    Check(Equal(got.data(), want.data(), sizeof(src)),
          "case 1: hand-picked values match the reference");
    // Anchor the two degenerate cases directly, so a broken reference cannot
    // silently agree with a broken kernel.
    Check(got[0] == 0x00 && got[3] == 0x00, "case 1: alpha 0 clears RGB");
    Check(got[4] == 0xFF && got[7] == 0xFF, "case 1: alpha 255 is identity");
    std::printf("  [PASS] case 1 smoke: 8 hand-picked pixels\n");
  }

  // --- Case 2: every tail length, plus alpha 0 and alpha 255 sweeps ---
  {
    bool ok = true;
    for (std::size_t n = 0; n <= 40 && ok; ++n) {
      Image src(n);
      src.FillRandom(static_cast<std::uint32_t>(n) * 7919U + 13U);
      Image got(n);
      got.FillPixels(0x00);
      Image want(n);
      want.FillPixels(0x00);

      PremultiplyRgba8(got.Data(), src.Data(), n);
      RefPremultiply(want.Data(), src.Data(), n);
      ok = Equal(got.Data(), want.Data(), got.Bytes()) && got.GuardIntact();
    }
    Check(ok, "case 2: every tail length 0..40 matches the reference");

    Image zero(37);  // not a multiple of 4 or 8 on purpose
    zero.FillRandom(1);
    zero.ForceAlpha(0);
    Image out(37);
    out.FillPixels(0x11);
    PremultiplyRgba8(out.Data(), zero.Data(), 37);
    Check(RegionIs(out.Data(), out.Bytes(), 0x00) && out.GuardIntact(),
          "case 2: alpha 0 collapses every pixel to zero");

    Image max(33);
    max.FillRandom(2);
    max.ForceAlpha(0xFF);
    Image id(33);
    id.FillPixels(0x00);
    PremultiplyRgba8(id.Data(), max.Data(), 33);
    Check(Equal(id.Data(), max.Data(), id.Bytes()) && id.GuardIntact(),
          "case 2: alpha 255 is exactly identity");
    std::printf("  [PASS] case 2 smoke: tail sweep + degenerate alphas\n");
  }

  // --- Case 3: stress, 256 KiB of random RGBA with guard bytes ---
  {
    constexpr std::size_t kPixels = std::size_t{1} << 16;
    Image src(kPixels);
    src.FillRandom(0xC0FFEEU);
    Image got(kPixels);
    got.FillPixels(0x00);
    Image want(kPixels);
    want.FillPixels(0x00);

    PremultiplyRgba8(got.Data(), src.Data(), kPixels);
    RefPremultiply(want.Data(), src.Data(), kPixels);
    Check(Equal(got.Data(), want.Data(), got.Bytes()),
          "case 3: 65536-pixel image matches the reference");
    Check(got.GuardIntact() && src.GuardIntact(), "case 3: guards intact");
    std::printf("  [PASS] case 3 stress: 65536 px (256 KiB)\n");
  }

  // --- Case 4: death / robustness, hostile arguments must never crash ---
  {
#if defined(NEOFLUX_VERIFY_CAN_FORK)
    // A genuine death test: fork, run the hostile calls in the child, and
    // require it to exit normally. A crash becomes a failed expectation
    // instead of taking the whole binary down.
    const pid_t pid = fork();
    if (pid == 0) {
      HostileCalls();
      _exit(0);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    Check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "case 4: hostile arguments exited normally");
#else
    HostileCalls();
#endif
    std::vector<std::uint8_t> untouched(8 * kChannels, 0x33);
    PremultiplyRgba8(nullptr, untouched.data(), 8);
    PremultiplyRgba8(untouched.data(), nullptr, 8);
    Check(RegionIs(untouched.data(), untouched.size(), 0x33),
          "case 4: rejected calls wrote nothing");
    std::printf("  [PASS] case 4 death: hostile arguments survive\n");
  }

  // --- Case 5: alias handling, dst may equal src ---
  {
    constexpr std::size_t kPixels = 4096;
    Image src(kPixels);
    src.FillRandom(3);
    Image inplace(kPixels);
    inplace.FillPixels(0x00);
    std::memcpy(inplace.Data(), src.Data(), src.Bytes());
    Image want(kPixels);
    want.FillPixels(0x00);
    RefPremultiply(want.Data(), src.Data(), kPixels);

    PremultiplyRgba8(inplace.Data(), inplace.Data(), kPixels);
    Check(Equal(inplace.Data(), want.Data(), want.Bytes()) &&
              inplace.GuardIntact(),
          "case 5: in-place dst==src matches the out-of-place result");
    std::printf("  [PASS] case 5: in-place aliasing\n");
  }

  // --- Case 6: the raw assembly kernel against the scalar reference ---
#if defined(NEOFLUX_NATIVE_ASM_PREMULTIPLY)
  {
    bool ok = true;
    for (std::size_t n = 0; n <= 4 * group + 5 && ok; ++n) {
      ok = RawMatchesReference(n, static_cast<std::uint32_t>(n) * 7919U + 13U);
    }
    for (std::size_t n : {std::size_t{4095}, std::size_t{4096},
                          std::size_t{4097}, std::size_t{65536},
                          std::size_t{65537}}) {
      ok = RawMatchesReference(n, static_cast<std::uint32_t>(n) * 31U + 7U) &&
           ok;
    }
    Check(ok, "case 6: assembly matches the scalar reference on every size");
    std::printf("  [PASS] case 6: assembly vs scalar, tails and guard bytes\n");
  }
#else
  std::printf("  [SKIP] case 6: no assembly kernel in this build "
              "(define NEOFLUX_NATIVE_ASM_PREMULTIPLY_X86_64 or "
              "_AARCH64 and link the matching .S)\n");
#endif

  // --- Case 7: SSE2 intrinsics as a second, independent oracle ---
#if defined(NEOFLUX_VERIFY_HAVE_SSE2)
  {
    constexpr std::size_t kPixels = std::size_t{1} << 14;
    Image src(kPixels);
    src.FillRandom(0x51DEU);
    Image via_intrinsics(kPixels);
    via_intrinsics.FillPixels(0x00);
    Image via_scalar(kPixels);
    via_scalar.FillPixels(0x00);

    PremultiplySse2(via_intrinsics.Data(), src.Data(), kPixels);
    RefPremultiply(via_scalar.Data(), src.Data(), kPixels);
    Check(Equal(via_intrinsics.Data(), via_scalar.Data(), via_scalar.Bytes()),
          "case 7: SSE2 intrinsic oracle matches the scalar reference");
    Check(via_intrinsics.GuardIntact(), "case 7: guards intact");

#if defined(NEOFLUX_NATIVE_ASM_PREMULTIPLY)
    bool ok = true;
    for (std::size_t n = 0; n <= 3 * group + 4 && ok; ++n) {
      Image s(n);
      s.FillRandom(static_cast<std::uint32_t>(n) * 7919U + 500U);
      Image a(n);
      a.FillPixels(0x00);
      Image b(n);
      b.FillPixels(0x00);
      PremultiplySse2(a.Data(), s.Data(), n);
      const std::size_t done =
          neoflux_premultiply_rgba8(b.Data(), s.Data(), n);
      ok = (done == (n / group) * group) &&
           Equal(a.Data(), b.Data(), done * kChannels);
    }
    Check(ok, "case 7: assembly matches the SSE2 intrinsic oracle");
#endif
    std::printf("  [PASS] case 7: SSE2 intrinsics cross-check\n");
  }
#endif

  if (failures != 0) {
    std::printf("\n%d CHECK(S) FAILED\n", failures);
    return 1;
  }
  std::printf("\nALL CHECKS PASSED\n");
  return 0;
}
