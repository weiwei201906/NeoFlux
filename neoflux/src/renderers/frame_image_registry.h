// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - frame_image_registry.h
//
// Process-wide id -> tgfx::Image registry. It is the plumbing that lets the
// render protocol carry an opaque image id instead of a GPU handle:
//
//   producer (media backend, render thread)
//       Image::MakeFrom(bitmap)  ->  Register()/Update()  ->  std::uint32_t id
//   consumer (TgfxRenderer::Execute, render thread)
//       Find(id)  ->  Canvas::drawImageRect(image, dest)
//
// RenderCommand is a flat POD copied through the SPSC ring queue, so it cannot
// own a std::shared_ptr<Image>; the id indirection is what keeps the command
// POD while still allowing any tgfx backend (OpenGL, Metal, Vulkan, D3D12) to
// composite the frame: the image is CPU-backed and tgfx uploads it through
// whichever backend is active.
//
// ID CONTRACT
//   - 0 is never a valid id. Register() returns 0 only for a null image.
//   - Ids are allocated monotonically and are NOT recycled, so a stale id can
//     never resolve to another producer's frame; it simply resolves to nullptr.
//   - An id stays valid (Find() returns the image) until Release() is called by
//     the producer that registered it. Find() on an unknown or released id is
//     not an error: it returns nullptr and the consumer skips the draw.
//   - A registered id keeps its image alive; the registry holds a strong
//     reference, so the producer may drop its own reference immediately.
//
// All methods are thread-safe (each takes an internal mutex) and may be called
// from any thread: producers register on the render thread and release from
// whichever thread runs the teardown, while consumers look up on the render
// thread.
//
// All method implementations are in frame_image_registry.cpp.
// =============================================================================

#ifndef NEOFLUX_RENDERERS_FRAME_IMAGE_REGISTRY_H_
#define NEOFLUX_RENDERERS_FRAME_IMAGE_REGISTRY_H_

#include <cstddef>
#include <cstdint>
#include <memory>

namespace tgfx {
class Image;
}  // namespace tgfx

namespace neoflux {

// Owns the process-wide frame-image id table. Never instantiated: every method
// is static and operates on a function-local (mutex-protected) registry.
class FrameImageRegistry {
 public:
  FrameImageRegistry() = delete;
  ~FrameImageRegistry() = delete;
  FrameImageRegistry(const FrameImageRegistry&) = delete;
  FrameImageRegistry& operator=(const FrameImageRegistry&) = delete;

  // The sentinel id meaning "no frame image". Never returned by Register().
  static constexpr std::uint32_t kInvalidImageId = 0;

  // Stores |image| under a fresh id and transfers a strong reference to it.
  // Any thread. Ownership: the registry keeps |image| alive until Release();
  // the caller may drop its own reference as soon as this returns. Failure
  // mode: returns kInvalidImageId for a null image (no exception is thrown, no
  // memory is allocated beyond the map node).
  [[nodiscard]] static std::uint32_t Register(std::shared_ptr<tgfx::Image> image);

  // Rebinds an existing |image_id| to |image| (used to publish a new frame
  // without churning ids). Any thread. Ownership: as Register(), the previous
  // image is released. Failure mode: returns false and leaves the table
  // unchanged when |image_id| is unknown or |image| is null.
  [[nodiscard]] static bool Update(std::uint32_t image_id,
                                   std::shared_ptr<tgfx::Image> image);

  // Returns the image bound to |image_id|, or nullptr when the id is unknown,
  // already released, or kInvalidImageId. Any thread. The caller shares
  // ownership for as long as it holds the returned pointer. Failure mode:
  // nullptr (never throws).
  [[nodiscard]] static std::shared_ptr<tgfx::Image> Find(std::uint32_t image_id);

  // Drops the registry's reference to |image_id|. Any thread. Idempotent:
  // releasing an unknown or already released id, or kInvalidImageId, is a
  // no-op. Failure mode: none (never throws).
  static void Release(std::uint32_t image_id) noexcept;

  // Number of live registrations. Any thread. Intended for tests and
  // observability, not for control flow.
  [[nodiscard]] static std::size_t Size() noexcept;
};

}  // namespace neoflux

#endif  // NEOFLUX_RENDERERS_FRAME_IMAGE_REGISTRY_H_
