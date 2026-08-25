// =============================================================================
// NeoFlux - widget_tree_stress_test.cpp
//
// Stress tests for the widget tree: deep nesting, wide branching, frequent
// rebuilds, layout performance, and hit-test performance. These tests catch
// stack overflow, memory leaks, and performance regressions that unit tests
// might miss.
// =============================================================================

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <vector>

#include "neoflux/widget/container.h"
#include "neoflux/widget/sized_box.h"
#include "neoflux/widget/widget.h"

namespace neoflux {
namespace {

// Helper: build a deeply nested Container tree of given depth.
// Each level has one child; the leaf is a SizedBox.
std::shared_ptr<Widget> BuildDeepTree(std::size_t depth) {
  std::shared_ptr<Widget> root = std::make_shared<Container>();
  Widget* current = root.get();
  for (std::size_t i = 0; i < depth; ++i) {
    auto child = std::make_shared<Container>();
    current->AddChild(child);
    current = child.get();
  }
  // Leaf: a fixed-size box so layout has something to measure.
  auto leaf = std::make_shared<SizedBox>(10.0F, 10.0F);
  current->AddChild(leaf);
  return root;
}

// Helper: build a wide tree: root with N children, each child is a Container.
std::shared_ptr<Widget> BuildWideTree(std::size_t child_count) {
  auto root = std::make_shared<Container>();
  for (std::size_t i = 0; i < child_count; ++i) {
    auto child = std::make_shared<Container>();
    child->SetDesiredSize(Size{.width = 10.0F, .height = 10.0F});
    root->AddChild(child);
  }
  return root;
}

// Helper: count total widgets in a tree (recursive, but used only for
// verification, not in hot paths).
std::size_t CountWidgets(const Widget& root) {
  std::size_t count = 1;
  for (const auto& child : root.GetChildren()) {
    if (child != nullptr) {
      count += CountWidgets(*child);
    }
  }
  return count;
}

// =============================================================================
// Deep nesting: ensure layout and hit-test do not stack overflow.
// =============================================================================

TEST(WidgetTreeStress, DeepNestingLayout) {
  // 500 levels is deep enough to stress the call stack but not overflow
  // on typical platforms (each frame is ~1-2KB, 500 * 2KB = 1MB).
  constexpr std::size_t kDepth = 500;
  auto root = BuildDeepTree(kDepth);

  // Layout should complete without stack overflow.
  root->PerformLayout(800.0F, 600.0F);

  // Verify the tree is intact.
  EXPECT_EQ(CountWidgets(*root), kDepth + 2);  // root + depth containers + leaf
}

TEST(WidgetTreeStress, DeepNestingHitTest) {
  constexpr std::size_t kDepth = 500;
  auto root = BuildDeepTree(kDepth);
  root->PerformLayout(800.0F, 600.0F);

  // Hit-test at origin should traverse the entire depth.
  auto hit = root->HitTest(Point{.x = 0.0F, .y = 0.0F});
  // The deepest container or the leaf should be hit (depending on layout).
  EXPECT_NE(hit, nullptr);
}

// =============================================================================
// Wide branching: many children at one level.
// =============================================================================

TEST(WidgetTreeStress, WideBranchingLayout) {
  constexpr std::size_t kChildren = 1000;
  auto root = BuildWideTree(kChildren);

  root->PerformLayout(800.0F, 600.0F);

  EXPECT_EQ(root->GetChildCount(), kChildren);
  EXPECT_EQ(CountWidgets(*root), kChildren + 1);
}

TEST(WidgetTreeStress, WideBranchingHitTest) {
  constexpr std::size_t kChildren = 1000;
  auto root = BuildWideTree(kChildren);
  root->PerformLayout(800.0F, 600.0F);

  // Hit-test should find a child (or the root if children have zero size).
  auto hit = root->HitTest(Point{.x = 5.0F, .y = 5.0F});
  EXPECT_NE(hit, nullptr);
}

// =============================================================================
// Frequent rebuild: create and destroy trees rapidly to detect leaks.
// =============================================================================

TEST(WidgetTreeStress, FrequentRebuildNoLeak) {
  // Build and destroy 100 trees of moderate size. If there is a leak in
  // Taitank node cleanup or shared_ptr cycles, this will show up in ASAN
  // or valgrind runs.
  for (int iteration = 0; iteration < 100; ++iteration) {
    auto root = BuildWideTree(100);
    root->PerformLayout(800.0F, 600.0F);
    // root goes out of scope, destructor frees Taitank nodes.
  }
  // If we reach here without ASAN errors, no obvious leak.
  SUCCEED();
}

TEST(WidgetTreeStress, FrequentRebuildDeepTree) {
  for (int iteration = 0; iteration < 50; ++iteration) {
    auto root = BuildDeepTree(200);
    root->PerformLayout(800.0F, 600.0F);
  }
  SUCCEED();
}

// =============================================================================
// Layout performance: measure time for large tree layout.
// These are not strict assertions (hardware-dependent), but serve as
// regression detectors. If layout suddenly takes 10x longer, something
// changed.
// =============================================================================

TEST(WidgetTreeStress, LayoutPerformanceLargeTree) {
  // Build a tree with ~10K widgets: 100 children, each with 100 grandchildren.
  auto root = std::make_shared<Container>();
  for (int i = 0; i < 100; ++i) {
    auto child = std::make_shared<Container>();
    for (int j = 0; j < 100; ++j) {
      auto grandchild = std::make_shared<Container>();
      grandchild->SetDesiredSize(Size{.width = 5.0F, .height = 5.0F});
      child->AddChild(grandchild);
    }
    root->AddChild(child);
  }

  const auto start = std::chrono::steady_clock::now();
  root->PerformLayout(1920.0F, 1080.0F);
  const auto end = std::chrono::steady_clock::now();
  const auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();

  EXPECT_EQ(CountWidgets(*root), 10101u);  // 1 root + 100 children + 10000 gc
  // Layout of 10K widgets should complete in reasonable time.
  // This is a soft assertion: log but do not fail on slow CI.
  if (elapsed_ms > 5000) {
    GTEST_LOG_(WARNING) << "Layout of 10K widgets took " << elapsed_ms
                        << "ms, may indicate regression";
  }
}

// =============================================================================
// Add/remove churn: repeatedly add and remove children to test Taitank
// node synchronization.
// =============================================================================

TEST(WidgetTreeStress, AddRemoveChurn) {
  auto root = std::make_shared<Container>();

  for (int iteration = 0; iteration < 1000; ++iteration) {
    // Add 10 children.
    for (int i = 0; i < 10; ++i) {
      root->AddChild(std::make_shared<Container>());
    }
    // Clear all children.
    root->ClearChildren();
    EXPECT_EQ(root->GetChildCount(), 0u);
  }

  // Final layout should work after churn.
  root->AddChild(std::make_shared<Container>());
  root->PerformLayout(800.0F, 600.0F);
  EXPECT_EQ(root->GetChildCount(), 1u);
}

// =============================================================================
// Mixed deep + wide tree: stress both dimensions simultaneously.
// =============================================================================

TEST(WidgetTreeStress, MixedDeepWideTree) {
  // 5 levels * 6 children = 1 + 6 + 36 + 216 + 1296 + 7776 = 9331 widgets.
  auto root = std::make_shared<Container>();

  // Build a 5-level tree with branching factor 6.
  std::vector<Widget*> current_level;
  current_level.push_back(root.get());
  for (int level = 0; level < 5; ++level) {
    std::vector<Widget*> next_level;
    for (auto* node : current_level) {
      for (int i = 0; i < 6; ++i) {
        auto child = std::make_shared<Container>();
        child->SetDesiredSize(Size{.width = 4.0F, .height = 4.0F});
        node->AddChild(child);
        next_level.push_back(child.get());
      }
    }
    current_level = std::move(next_level);
  }

  root->PerformLayout(1920.0F, 1080.0F);

  // Total widgets: 1 + 6 + 36 + 216 + 1296 + 7776 = 9331
  EXPECT_EQ(CountWidgets(*root), 9331u);
}

}  // namespace
}  // namespace neoflux
