// =============================================================================
// NeoFlux - route_registry_test.cpp
//
// Unit tests for the route registration system.
// =============================================================================

#include <neoflux/widget/route_registry.h>
#include <neoflux/widget/text.h>
#include <neoflux/widget/widget.h>

#include <memory>

#include <gtest/gtest.h>

namespace neoflux {
namespace {

class RouteRegistryTest : public ::testing::Test {
 protected:
  void SetUp() override { RouteRegistry::Instance().Clear(); }
  void TearDown() override { RouteRegistry::Instance().Clear(); }
};

TEST_F(RouteRegistryTest, RegisterAndHasRoute) {
  RouteRegistry::Instance().RegisterRoute(
      "/test", [](BuildContext& /*ctx*/) {
        return std::make_shared<Text>("test");
      });

  EXPECT_TRUE(RouteRegistry::Instance().HasRoute("/test"));
  EXPECT_FALSE(RouteRegistry::Instance().HasRoute("/nonexistent"));
}

TEST_F(RouteRegistryTest, BuildRouteReturnsWidget) {
  RouteRegistry::Instance().RegisterRoute(
      "/home", [](BuildContext& /*ctx*/) {
        return std::make_shared<Text>("Home Page");
      });

  BuildContext context(nullptr);
  auto widget = RouteRegistry::Instance().BuildRoute("/home", context);
  ASSERT_NE(widget, nullptr);
  EXPECT_EQ(widget->GetWidgetName(), "Text");
}

TEST_F(RouteRegistryTest, BuildUnknownRouteReturnsNull) {
  BuildContext context(nullptr);
  auto widget = RouteRegistry::Instance().BuildRoute("/missing", context);
  EXPECT_EQ(widget, nullptr);
}

TEST_F(RouteRegistryTest, RouteCount) {
  EXPECT_EQ(RouteRegistry::Instance().GetRouteCount(), 0U);

  RouteRegistry::Instance().RegisterRoute(
      "/a", [](BuildContext&) { return nullptr; });
  RouteRegistry::Instance().RegisterRoute(
      "/b", [](BuildContext&) { return nullptr; });

  EXPECT_EQ(RouteRegistry::Instance().GetRouteCount(), 2U);
}

TEST_F(RouteRegistryTest, OverwriteRoute) {
  int call_count = 0;
  RouteRegistry::Instance().RegisterRoute(
      "/x", [&](BuildContext&) {
        ++call_count;
        return nullptr;
      });

  RouteRegistry::Instance().RegisterRoute(
      "/x", [&](BuildContext&) {
        call_count += 10;
        return nullptr;
      });

  BuildContext context(nullptr);
  auto result = RouteRegistry::Instance().BuildRoute("/x", context);
  (void)result;
  EXPECT_EQ(call_count, 10);
}

}  // namespace
}  // namespace neoflux
