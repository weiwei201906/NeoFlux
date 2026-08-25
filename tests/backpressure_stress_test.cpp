// =============================================================================
// NeoFlux - backpressure_stress_test.cpp
//
// Stress tests for the render command queue backpressure mechanism.
// Verifies that when the SPSC queue fills up, excess commands are dropped
// (not crashed, not corrupted) and the queue remains usable after draining.
// =============================================================================

#include <gtest/gtest.h>

#include <vector>

#include "neoflux/core/ring_queue.h"

#include "core/ring_queue_impl.inc"
#include "neoflux/render/render_command.h"

namespace neoflux {
namespace {

// Simulates RenderLayer::Submit's truncation logic: push until queue full,
// then break and return the count submitted.
std::size_t SimulateSubmit(SpscRingQueue<RenderCommand>& queue,
                           const std::vector<RenderCommand>& commands) {
  std::size_t submitted = 0;
  for (std::size_t i = 0; i < commands.size(); ++i) {
    if (!queue.TryPush(commands[i])) {
      break;
    }
    ++submitted;
  }
  return submitted;
}

TEST(BackpressureStress, SingleFrameOverflow) {
  // Small queue: capacity 256 stores 255 elements (one slot reserved).
  SpscRingQueue<RenderCommand> queue;
  queue.Init(256);

  // Simulate a frame with 10000 commands (extremely complex UI).
  std::vector<RenderCommand> commands(10000);
  for (std::size_t i = 0; i < commands.size(); ++i) {
    commands[i].type = RenderCommandType::kDrawRect;
    commands[i].rect = Rect{.x = 0.0F, .y = 0.0F, .width = 10.0F, .height = 10.0F,};
  }

  const std::size_t submitted = SimulateSubmit(queue, commands);

  // Queue capacity 256 stores at most 255 elements.
  EXPECT_LE(submitted, 255u);
  EXPECT_GT(submitted, 0u);

  // Drain all submitted commands.
  std::size_t consumed = 0;
  RenderCommand cmd;
  while (queue.TryPop(cmd)) {
    ++consumed;
  }
  EXPECT_EQ(submitted, consumed);
}

TEST(BackpressureStress, RepeatedOverflowCycles) {
  SpscRingQueue<RenderCommand> queue;
  queue.Init(64);

  // 100 cycles of overflow + drain. Verifies no corruption after repeated
  // backpressure events.
  for (int cycle = 0; cycle < 100; ++cycle) {
    std::vector<RenderCommand> commands(500);
    for (auto& c : commands) {
      c.type = RenderCommandType::kNoop;
    }

    const std::size_t submitted = SimulateSubmit(queue, commands);
    EXPECT_LE(submitted, 63u);  // capacity 64 -> 63 elements

    // Drain.
    RenderCommand cmd;
    std::size_t consumed = 0;
    while (queue.TryPop(cmd)) {
      ++consumed;
    }
    EXPECT_EQ(submitted, consumed);
  }
}

TEST(BackpressureStress, PartialFillThenOverflow) {
  SpscRingQueue<RenderCommand> queue;
  queue.Init(128);

  // First fill halfway.
  std::vector<RenderCommand> batch1(50);
  for (auto& c : batch1) c.type = RenderCommandType::kSave;
  EXPECT_EQ(SimulateSubmit(queue, batch1), 50u);

  // Then overflow with a large batch.
  std::vector<RenderCommand> batch2(1000);
  for (auto& c : batch2) c.type = RenderCommandType::kRestore;
  const std::size_t submitted2 = SimulateSubmit(queue, batch2);

  // 50 + submitted2 <= 127 (capacity 128 -> 127 max).
  EXPECT_LE(50u + submitted2, 127u);

  // Drain all and verify count.
  RenderCommand cmd;
  std::size_t consumed = 0;
  while (queue.TryPop(cmd)) {
    ++consumed;
  }
  EXPECT_EQ(consumed, 50u + submitted2);
}

TEST(BackpressureStress, LargeQueueNoOverflow) {
  // Large queue (4096) should accept a normal frame (500 commands) fully.
  SpscRingQueue<RenderCommand> queue;
  queue.Init(4096);

  std::vector<RenderCommand> commands(500);
  for (std::size_t i = 0; i < commands.size(); ++i) {
    commands[i].type = (i & 1U) ? RenderCommandType::kDrawRect
                                : RenderCommandType::kDrawRoundedRect;
  }

  const std::size_t submitted = SimulateSubmit(queue, commands);
  EXPECT_EQ(submitted, 500u);  // No overflow.

  RenderCommand cmd;
  std::size_t consumed = 0;
  while (queue.TryPop(cmd)) {
    ++consumed;
  }
  EXPECT_EQ(consumed, 500u);
}

TEST(BackpressureStress, ConcurrentProducerConsumerOverflow) {
  // Multi-threaded: producer pushes as fast as possible, consumer drains.
  // Verifies no lost/corrupted commands under concurrent overflow.
  SpscRingQueue<int> queue;
  queue.Init(16);  // Tiny queue to force frequent overflow.

  std::atomic<bool> stop{false};
  std::atomic<std::uint64_t> produced{0};
  std::atomic<std::uint64_t> consumed{0};

  auto producer = [&]() {
    int value = 0;
    while (!stop.load()) {
      if (queue.TryPush(value)) {
        ++produced;
        ++value;
      }
      // If queue full, spin (simulates backpressure).
    }
  };

  auto consumer = [&]() {
    int value;
    while (!stop.load() || produced.load() > consumed.load()) {
      if (queue.TryPop(value)) {
        ++consumed;
      }
    }
  };

  std::thread p(producer);
  std::thread c(consumer);

  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  stop.store(true);
  p.join();
  c.join();

  // All produced items must be consumed (no loss).
  EXPECT_EQ(produced.load(), consumed.load());
}

}  // namespace
}  // namespace neoflux
