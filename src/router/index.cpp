// =============================================================================
// NeoFlux - router/index.cpp
//
// Registers every route of the host application. Each route maps a path (e.g.
// "/counter") to a view builder function defined in src/views/. This is the
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

namespace {

// Builds the default landing page (HomeView).
std::shared_ptr<neoflux::Widget> BuildHome(neoflux::BuildContext& context) {
  return std::make_shared<neoflux::HomeView>(context);
}

// Builds the counter demo page (CounterView).
std::shared_ptr<neoflux::Widget> BuildCounter(neoflux::BuildContext& context) {
  return std::make_shared<neoflux::CounterView>(context);
}

// Builds the about page (AboutView).
std::shared_ptr<neoflux::Widget> BuildAbout(neoflux::BuildContext& context) {
  return std::make_shared<neoflux::AboutView>(context);
}

}  // namespace

void RegisterRoutes() {
  auto& registry = neoflux::RouteRegistry::Instance();
  registry.RegisterRoute("/", BuildHome);
  registry.RegisterRoute("/counter", BuildCounter);
  registry.RegisterRoute("/about", BuildAbout);
}
