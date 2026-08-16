// =============================================================================
// NeoFlux - ring_queue_internal.h
//
// INTERNAL header: template method implementations for SpscRingQueue.
//
// This file is NOT part of the public API. It is included by:
//   - ring_queue.cpp (for framework explicit instantiations)
//   - test files (for test-only instantiations)
//
// The public ring_queue.h contains only the declaration.
// =============================================================================

#ifndef NEOFLUX_CORE_RING_QUEUE_INTERNAL_H_
#define NEOFLUX_CORE_RING_QUEUE_INTERNAL_H_

#include <new>
#include <type_traits>
#include <utility>

#include "neoflux/core/ring_queue.h"

namespace neoflux {

template <typename T, std::size_t Capacity>
SpscRingQueue<T, Capacity>::SpscRingQueue() : head_(0), tail_(0) {
  static_assert(std::is_move_constructible_v<T> ||
                    std::is_copy_constructible_v<T>,
                "T must be move-constructible or copy-constructible");
}

template <typename T, std::size_t Capacity>
SpscRingQueue<T, Capacity>::~SpscRingQueue() {
  T dummy{};
  while (TryPop(dummy)) {
    // popped and destroyed
  }
}

template <typename T, std::size_t Capacity>
bool SpscRingQueue<T, Capacity>::TryPush(T value) {
  const std::size_t head = head_.load(std::memory_order_relaxed);
  const std::size_t next_head = (head + 1) & kMask;

  if (next_head == tail_.load(std::memory_order_acquire)) {
    return false;
  }

  std::construct_at(Slot(head), std::move(value));
  head_.store(next_head, std::memory_order_release);
  return true;
}

template <typename T, std::size_t Capacity>
bool SpscRingQueue<T, Capacity>::TryPop(T& out) {
  const std::size_t tail = tail_.load(std::memory_order_relaxed);

  if (tail == head_.load(std::memory_order_acquire)) {
    return false;
  }

  out = std::move(*Slot(tail));
  std::destroy_at(Slot(tail));
  tail_.store((tail + 1) & kMask, std::memory_order_release);
  return true;
}

template <typename T, std::size_t Capacity>
bool SpscRingQueue<T, Capacity>::Empty() const noexcept {
  return head_.load(std::memory_order_acquire) ==
         tail_.load(std::memory_order_acquire);
}

template <typename T, std::size_t Capacity>
bool SpscRingQueue<T, Capacity>::Full() const noexcept {
  const std::size_t head = head_.load(std::memory_order_acquire);
  return ((head + 1) & kMask) == tail_.load(std::memory_order_acquire);
}

template <typename T, std::size_t Capacity>
std::size_t SpscRingQueue<T, Capacity>::Size() const noexcept {
  const std::size_t head = head_.load(std::memory_order_acquire);
  const std::size_t tail = tail_.load(std::memory_order_acquire);
  return (head - tail) & kMask;
}

template <typename T, std::size_t Capacity>
constexpr std::size_t SpscRingQueue<T, Capacity>::CapacityValue() noexcept {
  return Capacity;
}

template <typename T, std::size_t Capacity>
T* SpscRingQueue<T, Capacity>::Slot(std::size_t index) noexcept {
  return std::launder(reinterpret_cast<T*>(storage_ + index * sizeof(T)));
}

}  // namespace neoflux

#endif  // NEOFLUX_CORE_RING_QUEUE_INTERNAL_H_
