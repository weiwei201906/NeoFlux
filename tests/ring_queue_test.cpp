// =============================================================================
// NeoFlux - ring_queue_test.cpp
//
// Unit tests for the SPSC lock-free ring queue.
// =============================================================================

#include <neoflux/core/ring_queue.h>

#include "core/ring_queue_impl.inc"

#include <thread>
#include <vector>

#include <gtest/gtest.h>

namespace neoflux {
namespace {

TEST(SpscRingQueueTest, InitiallyEmpty) {
  SpscRingQueue<int> queue(16);
  EXPECT_TRUE(queue.Empty());
  EXPECT_FALSE(queue.Full());
  EXPECT_EQ(queue.Size(), 0U);
  EXPECT_EQ(queue.CapacityValue(), 16U);
}

TEST(SpscRingQueueTest, PushAndPop) {
  SpscRingQueue<int> queue(16);
  EXPECT_TRUE(queue.TryPush(42));
  EXPECT_FALSE(queue.Empty());
  EXPECT_EQ(queue.Size(), 1U);

  int value = 0;
  EXPECT_TRUE(queue.TryPop(value));
  EXPECT_EQ(value, 42);
  EXPECT_TRUE(queue.Empty());
}

TEST(SpscRingQueueTest, PopFromEmptyReturnsFalse) {
  SpscRingQueue<int> queue(16);
  int value = 0;
  EXPECT_FALSE(queue.TryPop(value));
}

TEST(SpscRingQueueTest, FillToCapacity) {
  SpscRingQueue<int> queue(4);
  // Capacity is 4, but one slot is reserved for the full/empty distinction,
  // so we can store Capacity - 1 = 3 elements.
  EXPECT_TRUE(queue.TryPush(1));
  EXPECT_TRUE(queue.TryPush(2));
  EXPECT_TRUE(queue.TryPush(3));
  EXPECT_TRUE(queue.Full());
  EXPECT_FALSE(queue.TryPush(4));
  EXPECT_EQ(queue.Size(), 3U);
}

TEST(SpscRingQueueTest, FIFOOrder) {
  SpscRingQueue<int> queue(8);
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
  SpscRingQueue<int> queue(4);
  // Push and pop to advance indices past the buffer end.
  for (int cycle = 0; cycle < 3; ++cycle) {
    EXPECT_TRUE(queue.TryPush(10LL * cycle));
    EXPECT_TRUE(queue.TryPush((10LL * cycle) + 1));
    int value1 = 0;
    int value2 = 0;
    EXPECT_TRUE(queue.TryPop(value1));
    EXPECT_TRUE(queue.TryPop(value2));
    EXPECT_EQ(value1, (10LL * cycle));
    EXPECT_EQ(value2, (10LL * cycle) + 1);
  }
  EXPECT_TRUE(queue.Empty());
}

TEST(SpscRingQueueTest, MoveOnlyType) {
  struct MoveOnly {
    int value = 0;
    MoveOnly() = default;
    explicit MoveOnly(int value) : value(value) {}
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&&) = default;
    MoveOnly& operator=(MoveOnly&&) = default;
    ~MoveOnly() = default;
  };

  SpscRingQueue<MoveOnly> queue(8);
  EXPECT_TRUE(queue.TryPush(MoveOnly(99)));
  MoveOnly out;
  EXPECT_TRUE(queue.TryPop(out));
  EXPECT_EQ(out.value, 99);
}

TEST(SpscRingQueueTest, SingleProducerSingleConsumer) {
  SpscRingQueue<int> queue(1024);
  constexpr int kCount = 10000;

  std::thread producer([&] {
    for (int i = 0; i < kCount; ++i) {
      while (!queue.TryPush(i)) {
        std::this_thread::yield();
      }
    }
  });

  std::thread consumer([&] {
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
