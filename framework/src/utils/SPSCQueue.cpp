#include "neoflux/utils/SPSCQueue.h"
#include "neoflux/engine/RenderEngine.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <memory>
#include <utility>

namespace neoflux::util {

template <typename T>
SPSCQueue<T>::SPSCQueue(const std::size_t capacity)
    : capacity_(std::max<std::size_t>(2U, std::bit_ceil(capacity))),
      mask_(capacity_ - 1U),
      buffer_(std::make_unique<T[]>(capacity_)) {}

template <typename T>
SPSCQueue<T>::~SPSCQueue() = default;

template <typename T>
bool SPSCQueue<T>::try_push(const T& value) {
  const std::size_t head = head_.load(std::memory_order_relaxed);
  const std::size_t next_head = (head + 1U) & mask_;
  if (const std::size_t tail = tail_.load(std::memory_order_acquire); next_head == tail) {
    return false;
  }

  std::construct_at(&buffer_[head], value);
  head_.store(next_head, std::memory_order_release);
  return true;
}

template <typename T>
bool SPSCQueue<T>::try_push(T&& value) {
  const std::size_t head = head_.load(std::memory_order_relaxed);
  const std::size_t next_head = (head + 1U) & mask_;
  if (const std::size_t tail = tail_.load(std::memory_order_acquire); next_head == tail) {
    return false;
  }

  std::construct_at(&buffer_[head], std::move(value));
  head_.store(next_head, std::memory_order_release);
  return true;
}

template <typename T>
template <typename... Args>
bool SPSCQueue<T>::emplace(Args&&... args) {
  const std::size_t head = head_.load(std::memory_order_relaxed);
  const std::size_t next_head = (head + 1U) & mask_;
  if (const std::size_t tail = tail_.load(std::memory_order_acquire); next_head == tail) {
    return false;
  }

  std::construct_at(&buffer_[head], std::forward<Args>(args)...);
  head_.store(next_head, std::memory_order_release);
  return true;
}

template <typename T>
bool SPSCQueue<T>::try_pop(T& out) {
  const std::size_t tail = tail_.load(std::memory_order_relaxed);
  if (const std::size_t head = head_.load(std::memory_order_acquire); tail == head) {
    return false;
  }

  out = std::move(buffer_[tail]);
  std::destroy_at(&buffer_[tail]);
  const std::size_t next_tail = (tail + 1U) & mask_;
  tail_.store(next_tail, std::memory_order_release);
  return true;
}

template <typename T>
bool SPSCQueue<T>::empty() const noexcept {
  const std::size_t head = head_.load(std::memory_order_acquire);
  const std::size_t tail = tail_.load(std::memory_order_acquire);
  return head == tail;
}

template <typename T>
std::size_t SPSCQueue<T>::capacity() const noexcept {
  return capacity_ - 1U;
}

template class SPSCQueue<int>;
template class SPSCQueue<engine::RenderEngine::RenderRequest>;

}  // namespace neoflux::util