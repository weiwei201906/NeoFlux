// =============================================================================
// NeoFlux - route_registry.cpp
//
// Implementation of the RouteRegistry singleton.
// =============================================================================

#include "neoflux/widget/route_registry.h"

#include <cstddef>
#include <memory>
#include <string_view>

#include <glog/logging.h>

#include "neoflux/widget/widget.h"

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
  const auto iter = routes_.find(std::string(route_name));
  if (iter == routes_.end()) {
    LOG(ERROR) << "Route not found: " << route_name;
    return nullptr;
  }
  return iter->second(context);
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
