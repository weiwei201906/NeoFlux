// =============================================================================
// NeoFlux - ring_queue_test.cpp
//
// Unit tests for the SPSC lock-free ring queue.
// =============================================================================

#include <neoflux/core/ring_queue_internal.h>

#include <thread>
#include <vector>

#include <gtest/gtest.h>

namespace neoflux {
namespace {

using TestQueue = SpscRingQueue<int, 16>;

TEST(SpscRingQueueTest, InitiallyEmpty) {
  TestQueue queue;
  EXPECT_TRUE(queue.Empty());
  EXPECT_FALSE(queue.Full());
  EXPECT_EQ(queue.Size(), 0u);
  EXPECT_EQ(queue.CapacityValue(), 16u);
}

TEST(SpscRingQueueTest, PushAndPop) {
  TestQueue queue;
  EXPECT_TRUE(queue.TryPush(42));
  EXPECT_FALSE(queue.Empty());
  EXPECT_EQ(queue.Size(), 1u);

  int value = 0;
  EXPECT_TRUE(queue.TryPop(value));
  EXPECT_EQ(value, 42);
  EXPECT_TRUE(queue.Empty());
}

TEST(SpscRingQueueTest, PopFromEmptyReturnsFalse) {
  TestQueue queue;
  int value = 0;
  EXPECT_FALSE(queue.TryPop(value));
}

TEST(SpscRingQueueTest, FillToCapacity) {
  SpscRingQueue<int, 4> queue;
  // Capacity is 4, but one slot is reserved for the full/empty distinction,
  // so we can store Capacity - 1 = 3 elements.
  EXPECT_TRUE(queue.TryPush(1));
  EXPECT_TRUE(queue.TryPush(2));
  EXPECT_TRUE(queue.TryPush(3));
  EXPECT_TRUE(queue.Full());
  EXPECT_FALSE(queue.TryPush(4));
  EXPECT_EQ(queue.Size(), 3u);
}

TEST(SpscRingQueueTest, FIFOOrder) {
  SpscRingQueue<int, 8> queue;
  for (int i = 0; i < 5; ++i) {
    EXPECT_TRUE(queue.TryPush(i));
  }
  for (int i = 0; i < 5; ++i) {
    int value = -1;
    EXPECT_TRUE(queue.TryPop(value));
    EXPECT_EQ(value, i);
  }
  EXPECT_TRUE(queue.Empty());
}

TEST(SpscRingQueueTest, WrapAround) {
  SpscRingQueue<int, 4> queue;
  // Push and pop to advance indices past the buffer end.
  for (int cycle = 0; cycle < 3; ++cycle) {
    EXPECT_TRUE(queue.TryPush(cycle * 10));
    EXPECT_TRUE(queue.TryPush(cycle * 10 + 1));
    int v1 = 0, v2 = 0;
    EXPECT_TRUE(queue.TryPop(v1));
    EXPECT_TRUE(queue.TryPop(v2));
    EXPECT_EQ(v1, cycle * 10);
    EXPECT_EQ(v2, cycle * 10 + 1);
  }
  EXPECT_TRUE(queue.Empty());
}

TEST(SpscRingQueueTest, MoveOnlyType) {
  struct MoveOnly {
    int value = 0;
    MoveOnly() = default;
    explicit MoveOnly(int v) : value(v) {}
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&&) = default;
    MoveOnly& operator=(MoveOnly&&) = default;
  };

  SpscRingQueue<MoveOnly, 8> queue;
  EXPECT_TRUE(queue.TryPush(MoveOnly(99)));
  MoveOnly out;
  EXPECT_TRUE(queue.TryPop(out));
  EXPECT_EQ(out.value, 99);
}

TEST(SpscRingQueueTest, SingleProducerSingleConsumer) {
  SpscRingQueue<int, 1024> queue;
  constexpr int kCount = 10000;

  std::thread producer([&]() {
    for (int i = 0; i < kCount; ++i) {
      while (!queue.TryPush(i)) {
        std::this_thread::yield();
      }
    }
  });

  std::thread consumer([&]() {
    for (int i = 0; i < kCount; ++i) {
      int value = -1;
      while (!queue.TryPop(value)) {
        std::this_thread::yield();
      }
      EXPECT_EQ(value, i);
    }
  });

  producer.join();
  consumer.join();
  EXPECT_TRUE(queue.Empty());
}

}  // namespace
}  // namespace neoflux
