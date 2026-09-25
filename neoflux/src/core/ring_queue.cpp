// =============================================================================
// NeoFlux - ring_queue.cpp
//
// SPSC lock-free ring queue template implementation and explicit
// instantiations. The template method definitions live here (not in a
// separate .inc) and are explicitly instantiated for the types used by the
// framework and tests.
//
// To use SpscRingQueue with a custom type from your own code, add an
// explicit instantiation for that type in one of your translation units and
// link against neoflux.
// =============================================================================

#include "neoflux/core/ring_queue.h"

#include <bit>
#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include "neoflux/render/render_command.h"

namespace neoflux {

template <typename T>
SpscRingQueue<T>::SpscRingQueue() = default;

template <typename T>
SpscRingQueue<T>::SpscRingQueue(const std::size_t capacity) {
  Init(capacity);
}

template <typename T>
void SpscRingQueue<T>::Init(const std::size_t capacity) {
  assert(capacity >= 2 && "SpscRingQueue capacity must be at least 2");
  assert(capacity_ == 0 && "SpscRingQueue::Init called twice");
  static_assert(std::is_move_constructible_v<T> ||
                    std::is_copy_constructible_v<T>,
                "T must be move-constructible or copy-constructible");
  // Guard against integer overflow in capacity * sizeof(T). If the requested
  // capacity would overflow size_t when multiplied by element size, clamp to
  // the maximum safe power of two. This prevents mmap of a truncated size
  // (which would cause out-of-bounds access on later Push).
  // Parens defeat the Windows min/max macro when windows.h is included
  // transitively.
  constexpr std::size_t kMaxSafeCapacity =
      // NOLINTNEXTLINE(readability-redundant-parentheses) - parens defeat the Windows min/max macro.
      (std::numeric_limits<std::size_t>::max)() / sizeof(T);
  const std::size_t safe_capacity =
      (capacity > kMaxSafeCapacity) ? kMaxSafeCapacity : capacity;

  // Round up to the next power of two so that index wrapping can use a
  // single bitwise AND (& mask_) instead of an integer modulo (%).
  // Integer division is ~20-40 cycles; bitwise AND is 1 cycle.
  capacity_ = std::bit_ceil(safe_capacity);
  mask_ = capacity_ - 1;
  // Allocate page-aligned storage via mmap/VirtualAlloc with a trailing
  // guard page. Out-of-bounds writes trigger SIGSEGV/access violation
  // instead of silently corrupting adjacent memory.
  storage_.emplace(capacity_ * sizeof(T), true);
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
  // storage_ is guaranteed engaged after Init(); operator* (unchecked) is
  // used intentionally to keep Slot noexcept -- value() could throw on the
  // noexcept path. NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  return std::launder(reinterpret_cast<T*>(
      static_cast<std::byte*>((*storage_).Data()) +
      (index * sizeof(T))));
}

// Explicit instantiation for the render command queue used by RenderLayer.
// The capacity is configured at runtime via the render_queue_capacity gflag.
template class SpscRingQueue<RenderCommand>;

// Instantiations used by unit tests and generic integer workloads.
template class SpscRingQueue<int>;
template class SpscRingQueue<std::uint64_t>;
template class SpscRingQueue<std::unique_ptr<int>>;

}  // namespace neoflux
