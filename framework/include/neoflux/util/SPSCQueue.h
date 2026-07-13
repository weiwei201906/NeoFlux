#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>

namespace neoflux {
namespace util {

// Simple lock-free SPSC ring buffer. Capacity is rounded up to next power of two.
template <typename T>
class SPSCQueue {
 public:
  explicit SPSCQueue(size_t capacity = 1024)
      : capacity_(nextPowerOfTwo(std::max<size_t>(1, capacity))),
        mask_(capacity_ - 1),
        buffer_(new T[capacity_]) {}

  ~SPSCQueue() = default;

  SPSCQueue(const SPSCQueue&) = delete;
  SPSCQueue& operator=(const SPSCQueue&) = delete;

  bool try_push(const T& value) {
    const size_t head = head_.load(std::memory_order_relaxed);
    const size_t next = head + 1;
    if (next - tail_.load(std::memory_order_acquire) > capacity_) {
      return false; // full
    }
    buffer_[head & mask_] = value;
    head_.store(next, std::memory_order_release);
    return true;
  }

  bool try_push(T&& value) {
    const size_t head = head_.load(std::memory_order_relaxed);
    const size_t next = head + 1;
    if (next - tail_.load(std::memory_order_acquire) > capacity_) {
      return false; // full
    }
    buffer_[head & mask_] = std::move(value);
    head_.store(next, std::memory_order_release);
    return true;
  }

  bool try_pop(T& out) {
    const size_t tail = tail_.load(std::memory_order_relaxed);
    if (tail == head_.load(std::memory_order_acquire)) {
      return false; // empty
    }
    out = buffer_[tail & mask_];
    tail_.store(tail + 1, std::memory_order_release);
    return true;
  }

 private:
  static size_t nextPowerOfTwo(size_t v) {
    --v;
    for (size_t i = 1; i < sizeof(size_t) * 8; i <<= 1) v |= v >> i;
    return ++v;
  }

  const size_t capacity_;
  const size_t mask_;
  std::unique_ptr<T[]> buffer_;
  std::atomic<size_t> head_{0};
  std::atomic<size_t> tail_{0};
};

} // namespace util
} // namespace neoflux
