// =============================================================================
// NeoFlux - widget_arena.h
//
// Arena allocator for Widget trees. Contiguous allocation improves cache
// locality during layout and paint traversal. Uses std::pmr::monotonic_buffer_resource
// for fast bump-pointer allocation with no per-object deallocation overhead.
//
// Usage:
//   WidgetArena arena;
//   auto root = arena.AllocateShared<Container>();
//   root->AddChild(arena.AllocateShared<Text>("Hello"));
//
// The arena owns the memory; shared_ptrs allocated from it keep the arena
// alive via a shared control block. When the last widget is destroyed, the
// arena's memory is released.
// =============================================================================

#ifndef NEOFLUX_WIDGET_WIDGET_ARENA_H_
#define NEOFLUX_WIDGET_WIDGET_ARENA_H_

#include <memory>
#include <memory_resource>
#include <utility>

namespace neoflux {

// Arena allocator for widget trees.
//
// Provides allocate_shared() that allocates both the widget object and its
// shared_ptr control block from a contiguous memory pool. This improves cache
// locality during tree traversal (layout, paint, hit-test) compared to
// individual heap allocations via std::make_shared.
//
// The arena is reference-counted: each shared_ptr allocated from the arena
// holds a reference to the arena via its control block. When the last widget
// is destroyed, the arena's memory is released.
class WidgetArena : public std::enable_shared_from_this<WidgetArena> {
 public:
  // Creates a new arena with the given initial buffer size (bytes).
  explicit WidgetArena(std::size_t initial_size = 65536);

  ~WidgetArena();

  // Non-copyable, non-movable (arena identity is stable).
  WidgetArena(const WidgetArena&) = delete;
  WidgetArena& operator=(const WidgetArena&) = delete;
  WidgetArena(WidgetArena&&) = delete;
  WidgetArena& operator=(WidgetArena&&) = delete;

  // Allocates a widget from the arena and returns a shared_ptr.
  // The shared_ptr's control block is also allocated from the arena.
  template <typename T, typename... Args>
  std::shared_ptr<T> AllocateShared(Args&&... args) {
    auto alloc = std::pmr::polymorphic_allocator<T>(&resource_);
    // allocate_shared uses the allocator for both the object and control block.
    return std::allocate_shared<T>(alloc, std::forward<Args>(args)...);
  }

  // Returns the number of bytes currently allocated from the arena.
  [[nodiscard]] std::size_t BytesAllocated() const noexcept;

  // Resets the arena, releasing all memory. Any shared_ptrs allocated from
  // this arena become dangling — only call when all widgets are destroyed.
  void Reset();

 private:
  std::pmr::monotonic_buffer_resource resource_;
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_WIDGET_ARENA_H_
