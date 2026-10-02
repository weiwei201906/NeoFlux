// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux user quick-start - media_view.h
//
// Demonstrates the MediaWidget (libmpv backend): a video player surface with
// play/pause and back controls. This is a reference screen; copy and adapt.
// =============================================================================

#ifndef NEOFLUX_APP_VIEWS_MEDIA_MEDIA_VIEW_H_
#define NEOFLUX_APP_VIEWS_MEDIA_MEDIA_VIEW_H_

#include <memory>

#include "neoflux/widgets/widget.h"

namespace neoflux_app {

// Builds the media player screen ("/media").
[[nodiscard]] std::shared_ptr<neoflux::Widget> BuildMediaView(
    neoflux::BuildContext& context);

}  // namespace neoflux_app

#endif  // NEOFLUX_APP_VIEWS_MEDIA_MEDIA_VIEW_H_
