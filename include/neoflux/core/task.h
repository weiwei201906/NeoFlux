// =============================================================================
// NeoFlux - Coroutine Task
//
// A minimal C++20 coroutine task type for the NeoFlux event loop.
// Tasks are lazily-started and resume on the EventLoop thread, enabling
// async-style code (animations, delayed work, sequential I/O) without
// manual callback chains.
//
// Usage:
//   Task<int> Compute() {
//     co_await Sleep(std::chrono::milliseconds(100));
//     co_return 42;
//   }
//
//   loop.Schedule(Compute());
// =============================================================================

#ifndef NEOFLUX_CORE_TASK_H_
#define NEOFLUX_CORE_TASK_H_

#include <chrono>
#include <coroutine>
#include <exception>
#include <utility>

namespace neoflux {

// Forward declaration to avoid circular include (event_loop.h includes task.h).
class EventLoop;

// A simple awaitable that suspends the coroutine and resumes it on the next
// event-loop tick. Used for yielding to the event loop.
struct YieldAwaitable {
  bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<>) const noexcept {}
  void await_resume() const noexcept {}
};

// Convenience factory: co_await Yield() suspends and resumes next frame.
[[nodiscard]] inline YieldAwaitable Yield() noexcept { return {}; }

// A time-based awaitable. Resumes the coroutine after the specified duration
// using the EventLoop's timer queue. The loop is resolved via
// EventLoop::Current() (thread-local), so Sleep() only works inside a
// coroutine running on an EventLoop.
struct SleepAwaitable {
  std::chrono::steady_clock::duration duration;

  bool await_ready() const noexcept { return duration.count() <= 0; }
  void await_suspend(std::coroutine_handle<> h);
  void await_resume() const noexcept {}
};

// Convenience factory: co_await Sleep(500ms) suspends for 500ms.
// Accepts any std::chrono duration type; converted to steady_clock duration.
template <typename Rep, typename Period>
[[nodiscard]] SleepAwaitable Sleep(
    std::chrono::duration<Rep, Period> duration) noexcept {
  return {std::chrono::duration_cast<std::chrono::steady_clock::duration>(
      duration)};
}

// Task<T> — a coroutine that produces a value of type T.
// Tasks are lazy: they do not start until awaited or explicitly scheduled.
template <typename T = void>
class Task {
 public:
  struct promise_type {
    T value{};
    std::exception_ptr exception;
    std::coroutine_handle<> continuation;

    Task get_return_object() {
      return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_always initial_suspend() const noexcept { return {}; }
    std::suspend_always final_suspend() const noexcept { return {}; }
    void unhandled_exception() { exception = std::current_exception(); }

    template <typename U>
    void return_value(U&& v) {
      value = std::forward<U>(v);
    }
  };

  Task() = default;
  explicit Task(std::coroutine_handle<promise_type> h) : handle_(h) {}

  Task(const Task&) = delete;
  Task& operator=(const Task&) = delete;

  Task(Task&& other) noexcept : handle_(other.handle_) {
    other.handle_ = nullptr;
  }
  Task& operator=(Task&& other) noexcept {
    if (this != &other) {
      Destroy();
      handle_ = other.handle_;
      other.handle_ = nullptr;
    }
    return *this;
  }

  ~Task() { Destroy(); }

  // Returns true if the coroutine has completed.
  [[nodiscard]] bool Done() const noexcept {
    return handle_ == nullptr || handle_.done();
  }

  // Resumes the coroutine. Safe to call only if not done.
  void Resume() {
    if (handle_ != nullptr && !handle_.done()) {
      handle_.resume();
    }
  }

  // Returns the result. Throws if the coroutine stored an exception.
  [[nodiscard]] T& Result() {
    if (handle_ != nullptr && handle_.promise().exception) {
      std::rethrow_exception(handle_.promise().exception);
    }
    return handle_.promise().value;
  }

  // Awaitable interface.
  bool await_ready() const noexcept {
    return handle_ != nullptr && handle_.done();
  }
  void await_suspend(std::coroutine_handle<> caller) {
    handle_.promise().continuation = caller;
    handle_.resume();
  }
  T& await_resume() { return Result(); }

  // Returns the raw coroutine handle (for EventLoop scheduling).
  [[nodiscard]] std::coroutine_handle<> Handle() const noexcept {
    return handle_;
  }

 private:
  void Destroy() {
    if (handle_ != nullptr) {
      handle_.destroy();
      handle_ = nullptr;
    }
  }

  std::coroutine_handle<promise_type> handle_;
};

// Specialisation for Task<void>.
template <>
class Task<void> {
 public:
  struct promise_type {
    std::exception_ptr exception;
    std::coroutine_handle<> continuation;

    Task get_return_object() {
      return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_always initial_suspend() const noexcept { return {}; }
    std::suspend_always final_suspend() const noexcept { return {}; }
    void unhandled_exception() { exception = std::current_exception(); }
    void return_void() {}
  };

  Task() = default;
  explicit Task(std::coroutine_handle<promise_type> h) : handle_(h) {}

  Task(const Task&) = delete;
  Task& operator=(const Task&) = delete;

  Task(Task&& other) noexcept : handle_(other.handle_) {
    other.handle_ = nullptr;
  }
  Task& operator=(Task&& other) noexcept {
    if (this != &other) {
      Destroy();
      handle_ = other.handle_;
      other.handle_ = nullptr;
    }
    return *this;
  }

  ~Task() { Destroy(); }

  [[nodiscard]] bool Done() const noexcept {
    return handle_ == nullptr || handle_.done();
  }

  void Resume() {
    if (handle_ != nullptr && !handle_.done()) {
      handle_.resume();
    }
  }

  void Result() {
    if (handle_ != nullptr && handle_.promise().exception) {
      std::rethrow_exception(handle_.promise().exception);
    }
  }

  bool await_ready() const noexcept {
    return handle_ != nullptr && handle_.done();
  }
  void await_suspend(std::coroutine_handle<> caller) {
    handle_.promise().continuation = caller;
    handle_.resume();
  }
  void await_resume() { Result(); }

  [[nodiscard]] std::coroutine_handle<> Handle() const noexcept {
    return handle_;
  }

 private:
  void Destroy() {
    if (handle_ != nullptr) {
      handle_.destroy();
      handle_ = nullptr;
    }
  }

  std::coroutine_handle<promise_type> handle_;
};

}  // namespace neoflux

#endif  // NEOFLUX_CORE_TASK_H_
