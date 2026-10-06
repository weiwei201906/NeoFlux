// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - frame_image_registry.cpp
//
// Implementation of the process-wide frame-image id table. See
// frame_image_registry.h for the id contract and the threading rules.
// =============================================================================

#include "renderers/frame_image_registry.h"

#include <mutex>
#include <unordered_map>
#include <utility>

#include "tgfx/core/Image.h"

namespace neoflux {

namespace {

// Guards |RegistryTable()| and |NextImageId()|. One mutex for both: the id
// counter is only ever touched while the table is being mutated.
std::mutex& RegistryMutex() {
  static std::mutex mutex;
  return mutex;
}

// The id -> image table. Function-local so it is constructed on first use and
// destroyed at exit, which keeps the registry usable from static initializers
// of other translation units.
std::unordered_map<std::uint32_t, std::shared_ptr<tgfx::Image>>& RegistryTable() {
  static std::unordered_map<std::uint32_t, std::shared_ptr<tgfx::Image>> table;
  return table;
}

// Returns the next free id. Caller MUST hold RegistryMutex(). Ids start at 1
// (0 is kInvalidImageId) and are never reused: after a wrap the counter skips
// back to 1, which can only collide after 2^32 registrations.
std::uint32_t AllocateImageId() {
  static std::uint32_t next_id = 1;
  const std::uint32_t id = next_id;
  next_id = (next_id == UINT32_MAX) ? 1U : next_id + 1U;
  return id;
}

}  // namespace

std::uint32_t FrameImageRegistry::Register(std::shared_ptr<tgfx::Image> image) {
  if (image == nullptr) {
    return kInvalidImageId;
  }
  const std::scoped_lock lock(RegistryMutex());
  const std::uint32_t image_id = AllocateImageId();
  RegistryTable().emplace(image_id, std::move(image));
  return image_id;
}

bool FrameImageRegistry::Update(std::uint32_t image_id,
                                std::shared_ptr<tgfx::Image> image) {
  if (image_id == kInvalidImageId || image == nullptr) {
    return false;
  }
  const std::scoped_lock lock(RegistryMutex());
  auto entry = RegistryTable().find(image_id);
  if (entry == RegistryTable().end()) {
    return false;
  }
  entry->second = std::move(image);
  return true;
}

std::shared_ptr<tgfx::Image> FrameImageRegistry::Find(std::uint32_t image_id) {
  if (image_id == kInvalidImageId) {
    return nullptr;
  }
  const std::scoped_lock lock(RegistryMutex());
  auto entry = RegistryTable().find(image_id);
  if (entry == RegistryTable().end()) {
    return nullptr;
  }
  return entry->second;
}

void FrameImageRegistry::Release(std::uint32_t image_id) noexcept {
  if (image_id == kInvalidImageId) {
    return;
  }
  const std::scoped_lock lock(RegistryMutex());
  RegistryTable().erase(image_id);
}

std::size_t FrameImageRegistry::Size() noexcept {
  const std::scoped_lock lock(RegistryMutex());
  return RegistryTable().size();
}

}  // namespace neoflux
