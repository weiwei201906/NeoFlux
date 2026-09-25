// =============================================================================
// NeoFlux - router/index.cpp
//
// Registers every route of the host application. Each route maps a path (e.g.
// "/counter") to a view builder lambda defined in src/views/. This is the
// single place to look when you want to know which pages exist and how they
// are reached.
// =============================================================================

#include "router/index.h"

#include <neoflux/widget/route_registry.h>
#include <neoflux/widget/widget.h>

#include "views/about_view.h"
#include "views/counter_view.h"
#include "views/home_view.h"

#include <memory>

void RegisterRoutes() {
  auto& registry = neoflux::RouteRegistry::Instance();

  // Views are wired up inline via builder lambdas; a route builder receives
  // the BuildContext and returns the root widget of the page.
  registry.RegisterRoute("/", [](neoflux::BuildContext& context) {
    return std::make_shared<neoflux::HomeView>(context);
  });
  registry.RegisterRoute("/counter", [](neoflux::BuildContext& context) {
    return std::make_shared<neoflux::CounterView>(context);
  });
  registry.RegisterRoute("/about", [](neoflux::BuildContext& context) {
    return std::make_shared<neoflux::AboutView>(context);
  });
}
