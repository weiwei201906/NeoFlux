#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

namespace neoflux::threading {

class ThreadPool {
 public:
  explicit ThreadPool(const std::size_t threadCount = std::thread::hardware_concurrency()) : running_(true) {
    for (std::size_t i = 0; i < threadCount; ++i) {
      workers_.emplace_back([this] {
        while (running_) {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [this] { return !running_ || !tasks_.empty(); });
            if (!running_ && tasks_.empty()) {
              return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
          }
          if (task) {
            task();
          }
        }
      });
    }
  }

  ~ThreadPool() {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      running_ = false;
    }
    condition_.notify_all();
    for (auto& worker : workers_) {
      if (worker.joinable()) {
        worker.join();
      }
    }
  }

  void submit(std::function<void()> task) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      tasks_.push(std::move(task));
    }
    condition_.notify_one();
  }

 private:
  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;
  std::mutex mutex_;
  std::condition_variable condition_;
  bool running_;
};

}  // namespace neoflux::threading