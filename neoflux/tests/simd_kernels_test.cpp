// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors

#include "neoflux/core/flags.h"
#include "native/simd_kernels.h"

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

namespace neoflux::native {
namespace {

TEST(SimdKernelsTest, NativeSimdFlagForcesScalarFallback) {
  const bool previous = FLAGS_native_simd;
  FLAGS_native_simd = false;

  std::vector<std::uint8_t> src = {
      255, 128, 64, 255, 100, 200, 50, 128};
  std::vector<std::uint8_t> dst(src.size());

  PremultiplyRgba8(dst.data(), src.data(), src.size() / 4);

  EXPECT_EQ(ActiveSimdLevel(), SimdLevel::kScalar);
  EXPECT_EQ(SimdGroupPixels(), 0U);
  EXPECT_EQ(dst[0], 255U);
  EXPECT_EQ(dst[1], 128U);
  EXPECT_EQ(dst[2], 64U);
  EXPECT_EQ(dst[3], 255U);
  EXPECT_EQ(dst[4], 50U);
  EXPECT_EQ(dst[5], 100U);
  EXPECT_EQ(dst[6], 25U);
  EXPECT_EQ(dst[7], 128U);

  FLAGS_native_simd = previous;
}

}  // namespace
}  // namespace neoflux::native
