// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - ring_queue.h
//
// Lock-free single-producer single-consumer (SPSC) bounded ring queue.
//
// This is a template, so the method definitions live at the bottom of this
// header (no separate .inc). The framework explicitly instantiates the
// RenderCommand specialization in src/core/ring_queue.cpp; other TUs (e.g.
// tests) instantiate their own specializations directly from this header.
// =============================================================================

#ifndef NEOFLUX_CORE_RING_QUEUE_H_
#define NEOFLUX_CORE_RING_QUEUE_H_

#include <atomic>
#include <bit>
#include <cassert>
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

#include "neoflux/core/config.h"

namespace neoflux {

namespace detail {

// Re-export for legacy callers; the source of truth is neoflux::config.
inline constexpr std::size_t kCacheLineSize = config::kCacheLineSize;

}  // namespace detail

// Lock-free SPSC bounded ring queue with runtime-configurable capacity.
//
// Template parameters:
//   T - Element type; must be movable (or copyable).
//
// The requested capacity is rounded up to the next power of two so that
// index wrapping can use a single bitwise AND (& mask_) instead of an
// integer modulo (%). One slot is reserved for the full/empty distinction,
// so the maximum number of storable elements is (capacity - 1).
//
// Thread safety: Exactly one producer thread and one consumer thread.
template <typename T>
class SpscRingQueue {
 public:
  using value_type = T;
  using size_type = std::size_t;

  // Constructs a queue with the given capacity (rounded up to power of two).
  // capacity must be >= 2.
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

  // Actual capacity (rounded up to power of two, including reserved slot).
  std::size_t capacity_;

  // Bitmask for index wrapping: capacity_ - 1 (all lower bits set).
  std::size_t mask_;

  // Producer index (cache-line aligned).
  alignas(detail::kCacheLineSize) std::atomic<std::size_t> head_;

  // Consumer index (cache-line aligned).
  alignas(detail::kCacheLineSize) std::atomic<std::size_t> tail_;
};

// ---------------------------------------------------------------------------
// Template method definitions (this is a template; definitions must be
// visible to every TU that instantiates a specialization).
// ---------------------------------------------------------------------------

template <typename T>
SpscRingQueue<T>::SpscRingQueue(const std::size_t capacity)
    : capacity_(0),
      mask_(0),
      head_(0),
      tail_(0) {
  assert(capacity >= 2 && "SpscRingQueue capacity must be at least 2");
  static_assert(std::is_move_constructible_v<T> ||
                    std::is_copy_constructible_v<T>,
                "T must be move-constructible or copy-constructible");

  // Round up to the next power of two so that index wrapping can use a
  // single bitwise AND (& mask_) instead of an integer modulo (%).
  // Integer division is ~20-40 cycles; bitwise AND is 1 cycle.
  capacity_ = std::bit_ceil(capacity);
  mask_ = capacity_ - 1;
  storage_.resize(capacity_ * sizeof(T));
}

template <typename T>
SpscRingQueue<T>::~SpscRingQueue() {
  // Destroy the live elements in place. This deliberately does not go through
  // TryPop(): that would need a `T dummy` to pop into, which would require T
  // to be default-constructible and assignable, contradicting the
  // move/copy-constructible contract asserted in the constructor.
  const std::size_t head = head_.load(std::memory_order_relaxed);
  for (std::size_t i = tail_.load(std::memory_order_relaxed); i != head;
       i = (i + 1) & mask_) {
    std::destroy_at(Slot(i));
  }
}

template <typename T>
bool SpscRingQueue<T>::TryPush(T value) {
  const std::size_t head = head_.load(std::memory_order_relaxed);
  const std::size_t next_head = (head + 1) & mask_;

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
  tail_.store((tail + 1) & mask_, std::memory_order_release);
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
  return ((head + 1) & mask_) == tail_.load(std::memory_order_acquire);
}

template <typename T>
std::size_t SpscRingQueue<T>::Size() const noexcept {
  const std::size_t head = head_.load(std::memory_order_acquire);
  const std::size_t tail = tail_.load(std::memory_order_acquire);
  return (head - tail) & mask_;
}

template <typename T>
std::size_t SpscRingQueue<T>::CapacityValue() const noexcept {
  return capacity_;
}

template <typename T>
T* SpscRingQueue<T>::Slot(std::size_t index) noexcept {
  return std::launder(
      reinterpret_cast<T*>(storage_.data() + (index * sizeof(T))));
}

}  // namespace neoflux

#endif  // NEOFLUX_CORE_RING_QUEUE_H_
