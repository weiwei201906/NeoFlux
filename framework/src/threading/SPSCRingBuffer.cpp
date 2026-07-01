#include <atomic>
#include <cstddef>
#include <memory>

namespace neoflux {
namespace threading {

template <typename T>
class SPSCRingBuffer {
 public:
  explicit SPSCRingBuffer(std::size_t size)
      : size_(size + 1), buffer_(new T[size_]), head_(0), tail_(0) {}

  bool push(const T& value) {
    auto next = (head_.load(std::memory_order_relaxed) + 1) % size_;
    if (next == tail_.load(std::memory_order_acquire)) {
      return false;
    }
    buffer_[head_.load(std::memory_order_relaxed)] = value;
    head_.store(next, std::memory_order_release);
    return true;
  }

  bool pop(T& value) {
    if (tail_.load(std::memory_order_acquire) == head_.load(std::memory_order_acquire)) {
      return false;
    }
    value = buffer_[tail_.load(std::memory_order_relaxed)];
    tail_.store((tail_.load(std::memory_order_relaxed) + 1) % size_, std::memory_order_release);
    return true;
  }

 private:
  std::size_t size_; 
  std::unique_ptr<T[]> buffer_;
  std::atomic<std::size_t> head_;
  std::atomic<std::size_t> tail_;
};

} // namespace threading
} // namespace neoflux
