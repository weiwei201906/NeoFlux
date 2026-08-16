// =============================================================================
// NeoFlux - ring_queue_impl.inc
//
// Template method implementations for SpscRingQueue.
//
// This is NOT a header file. It is included by:
//   - src/core/ring_queue.cpp        (framework explicit instantiations)
//   - tests/ring_queue_test.cpp      (test-only instantiations)
//
// The public declaration is in include/neoflux/core/ring_queue.h.
// =============================================================================
#pragma once

#include <cassert>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include "neoflux/core/ring_queue.h"

namespace neoflux {

template <typename T>
SpscRingQueue<T>::SpscRingQueue(std::size_t capacity)
    : storage_(capacity * sizeof(T)),
      capacity_(capacity),
      head_(0),
      tail_(0) {
  assert(capacity >= 2 && "SpscRingQueue capacity must be at least 2");
  static_assert(std::is_move_constructible_v<T> ||
                    std::is_copy_constructible_v<T>,
                "T must be move-constructible or copy-constructible");
}

template <typename T>
SpscRingQueue<T>::~SpscRingQueue() {
  T dummy{};
  while (TryPop(dummy)) {
    // popped and destroyed
  }
}

template <typename T>
bool SpscRingQueue<T>::TryPush(T value) {
  const std::size_t head = head_.load(std::memory_order_relaxed);
  const std::size_t next_head = (head + 1) % capacity_;

  if (next_head == tail_.load(std::memory_order_acquire)) {
    return false;
  }

  std::construct_at(Slot(head), std::move(value));
  head_.store(next_head, std::memory_order_release);
  return true;
}

template <typename T>
bool SpscRingQueue<T>::TryPop(T& out) {
  const std::size_t tail = tail_.load(std::memory_order_relaxed);

  if (tail == head_.load(std::memory_order_acquire)) {
    return false;
  }

  out = std::move(*Slot(tail));
  std::destroy_at(Slot(tail));
  tail_.store((tail + 1) % capacity_, std::memory_order_release);
  return true;
}

template <typename T>
bool SpscRingQueue<T>::Empty() const noexcept {
  return head_.load(std::memory_order_acquire) ==
         tail_.load(std::memory_order_acquire);
}

template <typename T>
bool SpscRingQueue<T>::Full() const noexcept {
  const std::size_t head = head_.load(std::memory_order_acquire);
  return ((head + 1) % capacity_) == tail_.load(std::memory_order_acquire);
}

template <typename T>
std::size_t SpscRingQueue<T>::Size() const noexcept {
  const std::size_t head = head_.load(std::memory_order_acquire);
  const std::size_t tail = tail_.load(std::memory_order_acquire);
  return (head - tail) % capacity_;
}

template <typename T>
std::size_t SpscRingQueue<T>::CapacityValue() const noexcept {
  return capacity_;
}

template <typename T>
T* SpscRingQueue<T>::Slot(std::size_t index) noexcept {
  return std::launder(reinterpret_cast<T*>(storage_.data() + (index * sizeof(T))));
}

}  // namespace neoflux
