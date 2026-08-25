// =============================================================================
// NeoFlux - widget_arena.cpp
//
// Implementation of WidgetArena, a pmr-based arena allocator for widget trees.
// =============================================================================

#include "neoflux/widget/widget_arena.h"

#include <cstddef>

namespace neoflux {

WidgetArena::WidgetArena(std::size_t initial_size)
    : resource_(initial_size) {}

WidgetArena::~WidgetArena() = default;

std::size_t WidgetArena::BytesAllocated() const noexcept {
  // monotonic_buffer_resource does not expose allocated bytes directly.
  // We track this approximately via the upstream resource's allocated count.
  // For a more precise count, we would need to wrap the upstream resource.
  // This returns 0 as a placeholder; actual tracking can be added via a
  // custom memory_resource wrapper if needed.
  return 0;
}

void WidgetArena::Reset() {
  resource_.release();
}

}  // namespace neoflux
