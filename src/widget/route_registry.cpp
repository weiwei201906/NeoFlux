// =============================================================================
// NeoFlux - route_registry.cpp
//
// Implementation of the RouteRegistry singleton.
// =============================================================================

#include "neoflux/widget/route_registry.h"

#include <glog/logging.h>

namespace neoflux {

RouteRegistry& RouteRegistry::Instance() {
  static RouteRegistry instance;
  return instance;
}

void RouteRegistry::RegisterRoute(std::string_view route_name,
                                  WidgetBuilder builder) {
  if (!builder) {
    LOG(WARNING) << "RegisterRoute called with null builder for route: "
                 << route_name;
    return;
  }
  routes_[std::string(route_name)] = std::move(builder);
  VLOG(1) << "Registered route: " << route_name;
}

std::shared_ptr<Widget> RouteRegistry::BuildRoute(
    std::string_view route_name, BuildContext& context) const {
  const auto it = routes_.find(std::string(route_name));
  if (it == routes_.end()) {
    LOG(ERROR) << "Route not found: " << route_name;
    return nullptr;
  }
  return it->second(context);
}

bool RouteRegistry::HasRoute(std::string_view route_name) const {
  return routes_.contains(std::string(route_name));
}

std::size_t RouteRegistry::GetRouteCount() const noexcept {
  return routes_.size();
}

void RouteRegistry::Clear() {
  routes_.clear();
  VLOG(1) << "RouteRegistry cleared";
}

}  // namespace neoflux
