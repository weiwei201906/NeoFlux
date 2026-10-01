// =============================================================================
// NeoFlux user quick-start - routers/index.h
//
// Central route registration entry. Add a new view under src/views/, then
// register its builder here. Routes are string names mapped to widget builders;
// Application::PushRoute(name) navigates to them.
// =============================================================================

#ifndef NEOFLUX_APP_ROUTERS_INDEX_H_
#define NEOFLUX_APP_ROUTERS_INDEX_H_

namespace neoflux_app {

// Registers all application routes with the framework RouteRegistry.
// Call once at startup before pushing the initial route.
void RegisterRoutes();

}  // namespace neoflux_app

#endif  // NEOFLUX_APP_ROUTERS_INDEX_H_
