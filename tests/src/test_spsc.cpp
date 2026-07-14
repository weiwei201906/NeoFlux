#include "gtest/gtest.h"

#include "neoflux/utils/SPSCQueue.h"

#include <thread>

using namespace neoflux::util;

TEST(SPSCQueueTest, PushPopSingleThread) {
  SPSCQueue<int> q(8);
  for (int i = 0; i < 8; ++i) {
    EXPECT_TRUE(q.try_push(i));
  }
  int v = -1;
  for (int i = 0; i < 8; ++i) {
    EXPECT_TRUE(q.try_pop(v));
    EXPECT_EQ(v, i);
  }
}

TEST(SPSCQueueTest, PushPopMultiThread) {
  SPSCQueue<int> q(1024);
  const int N = 10000;
  std::thread producer([&]() {
    for (int i = 0; i < N; ++i) {
      while (!q.try_push(i)) {}
    }
  });
  int count = 0;
  std::thread consumer([&]() {
    int v;
    while (count < N) {
      if (q.try_pop(v)) {
        ++count;
      }
    }
  });
  producer.join();
  consumer.join();
  EXPECT_EQ(count, N);
}
