// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux user quick-start - routers/index.cpp
// =============================================================================

#include "index.h"

#include "neoflux/widgets/route_registry.h"

#include "../views/home/home_view.h"

namespace neoflux_app {

void RegisterRoutes() {
  auto& registry = neoflux::RouteRegistry::Instance();
  registry.RegisterRoute("/", &BuildHomeView);
}

}  // namespace neoflux_app
