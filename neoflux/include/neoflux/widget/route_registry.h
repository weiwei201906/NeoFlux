// =============================================================================
// NeoFlux - route_registry.h
//
// Route-based widget registration and navigation.
//
// Widgets are registered by a string route name. The Application maintains
// a navigation stack; pushing a route name builds the corresponding widget
// and places it on top of the stack.
//
// Usage:
//   RouteRegistry::Instance().RegisterRoute("/home", [](BuildContext& ctx) {
//       return std::make_shared<MyHomeWidget>();
//   });
//   // Later:
//   context.PushRoute("/home");
// =============================================================================

#ifndef NEOFLUX_WIDGET_ROUTE_REGISTRY_H_
#define NEOFLUX_WIDGET_ROUTE_REGISTRY_H_

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "neoflux/core/noncopyable.h"
#include "neoflux/widget/widget.h"

namespace neoflux {

// Factory function that builds a widget for a given route.
using WidgetBuilder =
    std::function<std::shared_ptr<Widget>(BuildContext& context)>;

// Singleton registry mapping route names to widget builders.
//
// Routes are registered at startup (typically in main()) and looked up
// during navigation. The registry is thread-safe for registration before
// the event loop starts; lookups happen on the UI thread.
class RouteRegistry : public NonCopyable {
 public:
  // Returns the singleton instance.
  static RouteRegistry& Instance();

  // Registers a widget builder for the given route name.
  //
  // If a route with the same name already exists, it is overwritten.
  void RegisterRoute(std::string_view route_name, WidgetBuilder builder);

  // Builds and returns the widget for the given route name.
  //
  // Returns nullptr if no route is registered under that name.
  [[nodiscard]] std::shared_ptr<Widget> BuildRoute(
      std::string_view route_name, BuildContext& context) const;

  // Returns true if a route is registered under the given name.
  [[nodiscard]] bool HasRoute(std::string_view route_name) const;

  // Returns the number of registered routes.
  [[nodiscard]] std::size_t GetRouteCount() const noexcept;

  // Removes all registered routes (primarily for testing).
  void Clear();

 private:
  RouteRegistry() = default;

  std::unordered_map<std::string, WidgetBuilder> routes_;
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_ROUTE_REGISTRY_H_
