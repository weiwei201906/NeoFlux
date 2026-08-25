// =============================================================================
// NeoFlux - ring_queue_stress_test.cpp
//
// Stress tests for SpscRingQueue:
//   - Multi-threaded producer/consumer throughput
//   - FIFO ordering verification under concurrency
//   - Boundary conditions (min capacity, full/empty oscillation)
//   - Long-running stability (millions of operations)
//   - Move-only type correctness
//
// These tests are designed to catch race conditions, memory corruption,
// and integer overflow bugs that unit tests might miss.
// =============================================================================

#include "neoflux/core/ring_queue.h"

#include "core/ring_queue_impl.inc"

#include <atomic>
#include <cstdint>
#include <limits>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

namespace neoflux {

namespace {

// Number of items to push/pop in throughput tests.
constexpr std::size_t kStressCount = 1'000'000;

// Queue capacity for stress tests (power of two, moderate size).
constexpr std::size_t kStressCapacity = 1024;

}  // namespace

// Verifies FIFO ordering with a single producer and single consumer.
// Producer pushes 0..N-1; consumer checks each popped value == expected.
TEST(SpscRingQueueStress, FifoOrderingMultiThread) {
  SpscRingQueue<int> queue(kStressCapacity);
  std::atomic<bool> producer_done{false};
  std::atomic<std::size_t> consumed{0};

  std::thread producer([&queue]() {
    for (std::size_t i = 0; i < kStressCount; ++i) {
      while (!queue.TryPush(static_cast<int>(i))) {
        // Spin until space is available. SPSC queue is bounded; consumer
        // will eventually drain it.
      }
    }
  });

  std::thread consumer([&queue, &producer_done, &consumed]() {
    std::size_t expected = 0;
    while (expected < kStressCount) {
      int value = 0;
      if (queue.TryPop(value)) {
        EXPECT_EQ(value, static_cast<int>(expected));
        ++expected;
        ++consumed;
      }
    }
    producer_done = true;
  });

  producer.join();
  consumer.join();

  EXPECT_EQ(consumed.load(), kStressCount);
  EXPECT_TRUE(queue.Empty());
}

// Verifies no items are lost or duplicated under high contention.
// Uses a checksum: sum of pushed values must equal sum of popped values.
TEST(SpscRingQueueStress, NoLossNoDuplicate) {
  SpscRingQueue<std::uint64_t> queue(kStressCapacity);
  std::atomic<std::uint64_t> push_sum{0};
  std::atomic<std::uint64_t> pop_sum{0};

  std::thread producer([&queue, &push_sum]() {
    for (std::uint64_t i = 1; i <= kStressCount; ++i) {
      while (!queue.TryPush(i)) {
      }
      push_sum.fetch_add(i, std::memory_order_relaxed);
    }
  });

  std::thread consumer([&queue, &pop_sum]() {
    std::size_t received = 0;
    while (received < kStressCount) {
      std::uint64_t value = 0;
      if (queue.TryPop(value)) {
        pop_sum.fetch_add(value, std::memory_order_relaxed);
        ++received;
      }
    }
  });

  producer.join();
  consumer.join();

  EXPECT_EQ(push_sum.load(), pop_sum.load());
}

// Tests queue behavior at minimum capacity (2). With capacity 2, the queue
// can hold at most 1 element (one slot reserved for full/empty distinction).
TEST(SpscRingQueueStress, MinimumCapacity) {
  SpscRingQueue<int> queue(2);
  EXPECT_EQ(queue.CapacityValue(), 2U);

  // Should hold exactly 1 element.
  EXPECT_TRUE(queue.TryPush(42));
  EXPECT_FALSE(queue.TryPush(43));  // Full

  int value = 0;
  EXPECT_TRUE(queue.TryPop(value));
  EXPECT_EQ(value, 42);
  EXPECT_FALSE(queue.TryPop(value));  // Empty
}

// Tests rapid full/empty oscillation. Producer and consumer run at similar
// rates, causing the queue to frequently hit both boundaries.
TEST(SpscRingQueueStress, FullEmptyOscillation) {
  SpscRingQueue<int> queue(4);  // Very small queue to force oscillation.
  constexpr std::size_t kIterations = 100'000;

  std::thread producer([&queue]() {
    for (std::size_t i = 0; i < kIterations; ++i) {
      while (!queue.TryPush(static_cast<int>(i & 0xFF))) {
      }
    }
  });

  std::thread consumer([&queue]() {
    for (std::size_t i = 0; i < kIterations; ++i) {
      int value = 0;
      while (!queue.TryPop(value)) {
      }
      EXPECT_EQ(value, static_cast<int>(i & 0xFF));
    }
  });

  producer.join();
  consumer.join();
  EXPECT_TRUE(queue.Empty());
}

// Tests that the queue works correctly with move-only types under stress.
// Tracks live object count to detect leaks or double-frees.
struct MoveOnlyCounter {
  int value = 0;
  static std::atomic<int> live_count;

  MoveOnlyCounter() : value(-1) {}  // -1 = moved-from/invalid
  explicit MoveOnlyCounter(int v) : value(v) {
    live_count.fetch_add(1, std::memory_order_relaxed);
  }
  MoveOnlyCounter(const MoveOnlyCounter&) = delete;
  MoveOnlyCounter& operator=(const MoveOnlyCounter&) = delete;
  MoveOnlyCounter(MoveOnlyCounter&& other) noexcept : value(other.value) {
    other.value = -1;
    // Move ctor: new object becomes live, source was already live.
    // Net live_count unchanged (no add, no subtract).
  }
  MoveOnlyCounter& operator=(MoveOnlyCounter&& other) noexcept {
    if (this != &other) {
      if (value != -1) {
        // Replacing a live object: it dies.
        live_count.fetch_sub(1, std::memory_order_relaxed);
      }
      value = other.value;
      other.value = -1;
    }
    return *this;
  }
  ~MoveOnlyCounter() {
    if (value != -1) {
      live_count.fetch_sub(1, std::memory_order_relaxed);
    }
  }
};

std::atomic<int> MoveOnlyCounter::live_count{0};

TEST(SpscRingQueueStress, MoveOnlyTypeStress) {
  MoveOnlyCounter::live_count = 0;

  SpscRingQueue<MoveOnlyCounter> queue(256);
  constexpr std::size_t kCount = 50'000;

  std::thread producer([&queue]() {
    for (std::size_t i = 0; i < kCount; ++i) {
      // Construct once, retry push by moving. If push fails (queue full),
      // the value parameter is destroyed (live_count decremented), so we
      // reconstruct. This tests the queue's handling of move-only types
      // under backpressure.
      while (!queue.TryPush(MoveOnlyCounter(static_cast<int>(i)))) {
      }
    }
  });

  std::thread consumer([&queue]() {
    for (std::size_t i = 0; i < kCount; ++i) {
      MoveOnlyCounter item;
      while (!queue.TryPop(item)) {
      }
      EXPECT_EQ(item.value, static_cast<int>(i));
    }
  });

  producer.join();
  consumer.join();

  // All objects must be destroyed: no leaks, no double-frees.
  EXPECT_EQ(MoveOnlyCounter::live_count.load(), 0);
}

// Tests queue with a large (but realistic) capacity. Verifies that large
// allocations succeed and the queue remains functional.
TEST(SpscRingQueueStress, LargeCapacity) {
  constexpr std::size_t kLargeCap = 1 << 20;  // 1M slots
  SpscRingQueue<int> queue(kLargeCap);
  EXPECT_GE(queue.CapacityValue(), kLargeCap);

  // Fill and drain a portion to verify functionality.
  constexpr std::size_t kBatch = 10'000;
  for (std::size_t i = 0; i < kBatch; ++i) {
    EXPECT_TRUE(queue.TryPush(static_cast<int>(i)));
  }
  for (std::size_t i = 0; i < kBatch; ++i) {
    int v = 0;
    EXPECT_TRUE(queue.TryPop(v));
    EXPECT_EQ(v, static_cast<int>(i));
  }
  EXPECT_TRUE(queue.Empty());
}

}  // namespace neoflux
