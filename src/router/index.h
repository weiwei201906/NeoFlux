// =============================================================================
// NeoFlux - router/index.h
//
// Central route registry for the host application. Every route of the app is
// registered here in one place so navigation stays discoverable. Add a new
// route by writing a view (see views/) and calling RegisterRoute() below.
// =============================================================================

#ifndef NEOFLUX_QUICKSTART_ROUTER_INDEX_H_
#define NEOFLUX_QUICKSTART_ROUTER_INDEX_H_

// Registers all application routes with the framework's RouteRegistry.
// Must be called once, before Application::Init().
void RegisterRoutes();

#endif  // NEOFLUX_QUICKSTART_ROUTER_INDEX_H_
