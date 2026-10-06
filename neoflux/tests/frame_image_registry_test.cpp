// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - frame_image_registry_test.cpp
//
// Unit tests for the internal frame-image id table that carries media frames
// from a producer to TgfxRenderer without putting a std::shared_ptr in the flat
// RenderCommand struct. The registry is pure CPU bookkeeping plus tgfx's
// Bitmap/Image contract, so these tests need no GPU context and no media file.
// =============================================================================

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "renderers/frame_image_registry.h"

#include "tgfx/core/AlphaType.h"
#include "tgfx/core/Bitmap.h"
#include "tgfx/core/ColorType.h"
#include "tgfx/core/Image.h"
#include "tgfx/core/ImageInfo.h"

namespace neoflux {
namespace {

// Builds a CPU-backed tgfx image of the given size, filled with one color.
// Returns nullptr if tgfx cannot allocate the bitmap.
std::shared_ptr<tgfx::Image> MakeTestImage(int width, int height,
                                           std::uint32_t rgba) {
  tgfx::Bitmap bitmap;
  if (!bitmap.allocPixels(width, height, false, false)) {
    return nullptr;
  }
  const auto info = tgfx::ImageInfo::Make(
      width, height, tgfx::ColorType::RGBA_8888, tgfx::AlphaType::Unpremultiplied);
  const std::vector<std::uint32_t> pixels(
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height), rgba);
  if (!bitmap.writePixels(info, pixels.data(), 0, 0)) {
    return nullptr;
  }
  return tgfx::Image::MakeFrom(bitmap);
}

// ---------------------------------------------------------------------------
// ID contract.
// ---------------------------------------------------------------------------

TEST(FrameImageRegistryTest, RegisterReturnsNonZeroUniqueIds) {
  auto first = MakeTestImage(4, 4, 0xFF0000FFU);
  auto second = MakeTestImage(4, 4, 0x00FF00FFU);
  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);

  const std::uint32_t first_id = FrameImageRegistry::Register(first);
  const std::uint32_t second_id = FrameImageRegistry::Register(second);
  EXPECT_NE(first_id, FrameImageRegistry::kInvalidImageId);
  EXPECT_NE(second_id, FrameImageRegistry::kInvalidImageId);
  // Ids are never recycled, so two live registrations can never collide.
  EXPECT_NE(first_id, second_id);

  FrameImageRegistry::Release(first_id);
  FrameImageRegistry::Release(second_id);
}

TEST(FrameImageRegistryTest, RegisterRejectsNullImage) {
  EXPECT_EQ(FrameImageRegistry::Register(nullptr),
            FrameImageRegistry::kInvalidImageId);
}

TEST(FrameImageRegistryTest, FindReturnsTheRegisteredImage) {
  auto image = MakeTestImage(4, 4, 0xFF0000FFU);
  ASSERT_NE(image, nullptr);
  const std::uint32_t image_id = FrameImageRegistry::Register(image);
  ASSERT_NE(image_id, FrameImageRegistry::kInvalidImageId);

  const auto found = FrameImageRegistry::Find(image_id);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found.get(), image.get());

  FrameImageRegistry::Release(image_id);
  EXPECT_EQ(FrameImageRegistry::Find(image_id), nullptr);
}

TEST(FrameImageRegistryTest, FindUnknownOrInvalidIdReturnsNull) {
  EXPECT_EQ(FrameImageRegistry::Find(FrameImageRegistry::kInvalidImageId), nullptr);
  // An id that was never handed out by Register().
  EXPECT_EQ(FrameImageRegistry::Find(0xFFFFFFF0U), nullptr);
}

TEST(FrameImageRegistryTest, UpdateRebindsTheIdToTheNewImage) {
  auto first = MakeTestImage(4, 4, 0xFF0000FFU);
  auto second = MakeTestImage(4, 4, 0x00FF00FFU);
  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);

  const std::uint32_t image_id = FrameImageRegistry::Register(first);
  ASSERT_NE(image_id, FrameImageRegistry::kInvalidImageId);

  // Republishing under the same id is how the player hands over the next frame
  // without ever invalidating the id a pending draw command may reference.
  ASSERT_TRUE(FrameImageRegistry::Update(image_id, second));
  const auto found = FrameImageRegistry::Find(image_id);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found.get(), second.get());

  FrameImageRegistry::Release(image_id);
}

TEST(FrameImageRegistryTest, UpdateRejectsUnknownIdAndNullImage) {
  auto image = MakeTestImage(4, 4, 0xFF0000FFU);
  ASSERT_NE(image, nullptr);

  EXPECT_FALSE(FrameImageRegistry::Update(0xFFFFFFF0U, image));
  const std::uint32_t image_id = FrameImageRegistry::Register(image);
  ASSERT_NE(image_id, FrameImageRegistry::kInvalidImageId);
  EXPECT_FALSE(FrameImageRegistry::Update(image_id, nullptr));
  // The rejected update must leave the previous binding intact.
  EXPECT_EQ(FrameImageRegistry::Find(image_id).get(), image.get());

  FrameImageRegistry::Release(image_id);
}

TEST(FrameImageRegistryTest, ReleaseIsIdempotentAndDropsTheImage) {
  const std::size_t initial_size = FrameImageRegistry::Size();

  auto image = MakeTestImage(4, 4, 0xFF0000FFU);
  ASSERT_NE(image, nullptr);
  const std::uint32_t image_id = FrameImageRegistry::Register(image);
  ASSERT_NE(image_id, FrameImageRegistry::kInvalidImageId);
  EXPECT_EQ(FrameImageRegistry::Size(), initial_size + 1U);

  FrameImageRegistry::Release(image_id);
  EXPECT_EQ(FrameImageRegistry::Size(), initial_size);
  EXPECT_EQ(FrameImageRegistry::Find(image_id), nullptr);
  // Releasing twice, or releasing the invalid id, must not fault or conflict.
  FrameImageRegistry::Release(image_id);
  FrameImageRegistry::Release(FrameImageRegistry::kInvalidImageId);
  EXPECT_EQ(FrameImageRegistry::Size(), initial_size);

  // The registry's reference was dropped, so releasing it must have left the
  // caller's shared_ptr as the only owner.
  EXPECT_EQ(image.use_count(), 1);
}

// ---------------------------------------------------------------------------
// Pixel stability: the property the media producer relies on.
//
// Image::MakeFrom(const Bitmap&) shares the bitmap's pixels and tgfx detaches
// them again on the next write ("the Bitmap will allocate new internal pixel
// memory and copy the original pixels into it if there is a subsequent call of
// pixel writing to the Bitmap. Therefore, the content of the returned Image
// will always be the same"). MpvMediaPlayer overwrites its frame bitmap every
// frame, so this behaviour is what keeps an already-published frame stable
// while it is still queued for drawing.
// ---------------------------------------------------------------------------

TEST(FrameImageRegistryTest, ImageFromBitmapKeepsStablePixels) {
  tgfx::Bitmap bitmap;
  ASSERT_TRUE(bitmap.allocPixels(4, 4, false, false));

  const void* const before = bitmap.lockPixels();
  bitmap.unlockPixels();
  ASSERT_NE(before, nullptr);

  auto image = tgfx::Image::MakeFrom(bitmap);
  ASSERT_NE(image, nullptr);
  EXPECT_EQ(image->width(), 4);
  EXPECT_EQ(image->height(), 4);

  // The writable pixels must have been detached from the image's copy.
  const void* const after = bitmap.lockPixels();
  bitmap.unlockPixels();
  ASSERT_NE(after, nullptr);
  EXPECT_NE(before, after);
}

}  // namespace
}  // namespace neoflux
