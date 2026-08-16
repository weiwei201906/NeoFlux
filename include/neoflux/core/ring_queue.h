// =============================================================================
// NeoFlux - ring_queue.h
//
// Lock-free single-producer single-consumer (SPSC) bounded ring queue.
//
// This header contains ONLY the template class declaration. Method
// implementations live in src/core/ring_queue_impl.inc and are explicitly
// instantiated in src/core/ring_queue.cpp for framework types.
//
// To use this queue with a custom type, include ring_queue_impl.inc and
// explicitly instantiate the required specialization.
// =============================================================================

#ifndef NEOFLUX_CORE_RING_QUEUE_H_
#define NEOFLUX_CORE_RING_QUEUE_H_

#include <atomic>
#include <cstddef>
#include <vector>

namespace neoflux {

namespace detail {

// L1 cache line size on most architectures.
inline constexpr std::size_t kCacheLineSize = 64;

}  // namespace detail

// Lock-free SPSC bounded ring queue with runtime-configurable capacity.
//
// Template parameters:
//   T - Element type; must be movable (or copyable).
//
// The capacity is specified at construction time. One slot is reserved
// for the full/empty distinction, so the maximum number of storable
// elements is (capacity - 1).
//
// Thread safety: Exactly one producer thread and one consumer thread.
template <typename T>
class SpscRingQueue {
 public:
  using value_type = T;
  using size_type = std::size_t;

  // Constructs a queue with the given capacity. capacity must be >= 2.
  explicit SpscRingQueue(std::size_t capacity);
  ~SpscRingQueue();

  // Non-copyable, non-movable (contains atomic members and raw storage).
  SpscRingQueue(const SpscRingQueue&) = delete;
  SpscRingQueue& operator=(const SpscRingQueue&) = delete;
  SpscRingQueue(SpscRingQueue&&) = delete;
  SpscRingQueue& operator=(SpscRingQueue&&) = delete;

  // Pushes an element. Producer thread only. Returns false if full.
  bool TryPush(T value);

  // Pops an element. Consumer thread only. Returns false if empty.
  bool TryPop(T& out);

  // Returns true if empty (snapshot).
  [[nodiscard]] bool Empty() const noexcept;

  // Returns true if full (snapshot).
  [[nodiscard]] bool Full() const noexcept;

  // Returns current size (approximate).
  [[nodiscard]] std::size_t Size() const noexcept;

  // Returns the maximum capacity (including the reserved slot).
  [[nodiscard]] std::size_t CapacityValue() const noexcept;

 private:
  // Returns pointer to the raw storage slot at the given index.
  T* Slot(std::size_t index) noexcept;

  // Raw storage for elements (allocated to capacity * sizeof(T)).
  std::vector<std::byte> storage_;

  // Requested capacity (number of slots including the reserved one).
  std::size_t capacity_;

  // Producer index (cache-line aligned).
  alignas(detail::kCacheLineSize) std::atomic<std::size_t> head_;

  // Consumer index (cache-line aligned).
  alignas(detail::kCacheLineSize) std::atomic<std::size_t> tail_;
};

}  // namespace neoflux

#endif  // NEOFLUX_CORE_RING_QUEUE_H_
