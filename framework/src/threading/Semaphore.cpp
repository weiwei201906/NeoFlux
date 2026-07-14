#include <condition_variable>
#include <mutex>
#include "neoflux/core/BuildContext.h"

namespace neoflux::threading {

class Semaphore {
 public:
  explicit Semaphore(const int count = 0) : count_(count) {}

  void notify() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++count_;
    condition_.notify_one();
  }

  void wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [this] { return count_ > 0; });
    --count_;
  }

 private:
  std::mutex mutex_;
  std::condition_variable condition_;
  int count_;
};

}  // namespace neoflux::threading