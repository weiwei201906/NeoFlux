// =============================================================================
// NeoFlux - object_pool_test.cpp
//
// Unit tests for ObjectPool: acquire/release, pool overflow, thread safety.
// =============================================================================

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

#include "neoflux/core/object_pool.h"

namespace neoflux {
namespace {

// Simple test type with default constructor and a value.
struct TestObject {
  int value = 0;
  TestObject() = default;
};

TEST(ObjectPoolTest, AcquireFromEmptyPoolCreatesNew) {
  ObjectPool<TestObject> pool(8);
  EXPECT_EQ(pool.Size(), 0U);

  auto obj = pool.Acquire();
  ASSERT_NE(obj, nullptr);
  EXPECT_EQ(obj->value, 0);
  // Object is in use, not in pool.
  EXPECT_EQ(pool.Size(), 0U);
}

TEST(ObjectPoolTest, ReleaseReturnsToPool) {
  ObjectPool<TestObject> pool(8);
  {
    auto obj = pool.Acquire();
    obj->value = 42;
  }  // obj released -> returned to pool
  EXPECT_EQ(pool.Size(), 1U);

  // Acquire again should reuse the pooled object.
  auto obj2 = pool.Acquire();
  ASSERT_NE(obj2, nullptr);
  EXPECT_EQ(pool.Size(), 0U);
}

TEST(ObjectPoolTest, PoolOverflowDestroysObject) {
  ObjectPool<TestObject> pool(2);
  {
    auto a = pool.Acquire();
    auto b = pool.Acquire();
    auto c = pool.Acquire();
  }  // 3 objects released, but pool max is 2 -> 1 destroyed
  EXPECT_EQ(pool.Size(), 2U);
}

TEST(ObjectPoolTest, ClearEmptiesPool) {
  ObjectPool<TestObject> pool(8);
  {
    auto a = pool.Acquire();
    auto b = pool.Acquire();
  }
  EXPECT_EQ(pool.Size(), 2U);
  pool.Clear();
  EXPECT_EQ(pool.Size(), 0U);
}

TEST(ObjectPoolTest, MaxSizeReturnsConfiguredValue) {
  ObjectPool<TestObject> pool(32);
  EXPECT_EQ(pool.MaxSize(), 32U);
}

TEST(ObjectPoolTest, MultipleAcquireReleaseCycles) {
  ObjectPool<TestObject> pool(4);
  for (int i = 0; i < 100; ++i) {
    auto obj = pool.Acquire();
    obj->value = i;
    EXPECT_EQ(obj->value, i);
  }
  // After all cycles, at most max_size objects remain in pool.
  EXPECT_LE(pool.Size(), 4U);
}

TEST(ObjectPoolTest, ThreadSafety) {
  ObjectPool<TestObject> pool(16);
  std::atomic<int> created{0};
  std::atomic<int> errors{0};

  auto worker = [&]() {
    for (int i = 0; i < 1000; ++i) {
      auto obj = pool.Acquire();
      if (obj == nullptr) {
        errors++;
      }
    }
  };

  std::vector<std::thread> threads;
  for (int t = 0; t < 4; ++t) {
    threads.emplace_back(worker);
  }
  for (auto& th : threads) {
    th.join();
  }

  EXPECT_EQ(errors.load(), 0);
  EXPECT_LE(pool.Size(), 16U);
}

}  // namespace
}  // namespace neoflux
