#pragma once

#include <atomic>
#include <cstddef>
#include <memory>

namespace neoflux::util {

// Simple lock-free SPSC ring buffer. Capacity is rounded up to next power of two.
template <typename T>
class SPSCQueue {
 public:
  explicit SPSCQueue(std::size_t capacity = 1024);
  ~SPSCQueue();

  SPSCQueue(const SPSCQueue&) = delete;
  SPSCQueue& operator=(const SPSCQueue&) = delete;

  bool try_push(const T&);

  bool try_push(T&&);

  template <typename... Args>
  bool emplace(Args&&... args);

  bool try_pop(T&);

  [[nodiscard]] bool empty() const noexcept;

  [[nodiscard]] std::size_t capacity() const noexcept;

 private:
  const size_t capacity_;
  const size_t mask_;
  std::unique_ptr<T[]> buffer_;
  std::atomic<size_t> head_{0};
  std::atomic<size_t> tail_{0};
};

}  // namespace neoflux::util