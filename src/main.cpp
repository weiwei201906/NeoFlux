// =============================================================================
// NeoFlux user quick-start - main.cpp
//
// Minimal application entry point:
//   1. Register your routes (see src/routers/index.cpp).
//   2. Create and initialize the framework Application.
//   3. Push the initial route and run the event loop.
//
// This file is intentionally tiny; put your business screens under src/views/.
// =============================================================================

#include <glog/logging.h>

#include "neoflux/apps/application.h"
#include "neoflux/widgets/route_registry.h"

#include "routers/index.h"

int main(int argc, char** argv) {
  // 1. Register application routes.
  neoflux_app::RegisterRoutes();

  // 2. Create and initialize the application (creates the desktop window).
  neoflux::Application app;
  if (!app.Init(argc, argv, 960, 640, "NeoFlux Quick Start")) {
    LOG(ERROR) << "Failed to initialize NeoFlux application";
    return 1;
  }

  // 3. Push the initial route and run the event loop (blocks until closed).
  app.PushRoute("/");
  app.Run();

  return 0;
}
