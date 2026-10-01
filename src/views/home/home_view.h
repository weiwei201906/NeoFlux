// =============================================================================
// NeoFlux user quick-start - home_view.h
//
// The home screen of your app. Each view lives in its own subdirectory under
// src/views/<view_name>/ with a <view_name>_view.h / <view_name>_view.cpp pair.
//
// This is YOUR code: edit it to build the first screen of your application.
// =============================================================================

#ifndef NEOFLUX_APP_VIEWS_HOME_HOME_VIEW_H_
#define NEOFLUX_APP_VIEWS_HOME_HOME_VIEW_H_

#include <memory>

#include "neoflux/widgets/widget.h"

namespace neoflux_app {

// Builds the widget tree for the home route ("/").
[[nodiscard]] std::shared_ptr<neoflux::Widget> BuildHomeView(
    neoflux::BuildContext& context);

}  // namespace neoflux_app

#endif  // NEOFLUX_APP_VIEWS_HOME_HOME_VIEW_H_
