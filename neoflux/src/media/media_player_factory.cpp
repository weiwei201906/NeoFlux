// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - media_player_factory.cpp
//
// Factory function that creates the platform-appropriate MediaPlayer.
//
// ARCHITECTURE: media playback is bound to the OpenGL backend module. libmpv
// decodes frames into GL textures and TgfxRenderer composites them via tgfx's
// GL texture interop (kDrawTexture) -- a path that exists only on the OpenGL
// backend. The build therefore defines NEOFLUX_HAS_MPV only when
// TGFX_USE_OPENGL is the active tgfx backend (see neoflux/CMakeLists.txt).
// On any other backend:
//   - mpv is not probed, downloaded, or linked;
//   - MpvMediaPlayer compiles as a no-op stub (UpdateTexture() returns 0);
//   - MediaWidget::Paint() draws its "No media loaded" placeholder.
// The MediaPlayer interface and the kDrawTexture protocol command stay
// backend-agnostic on purpose: a future backend can implement its own texture
// importer without touching this module or the render protocol.
// =============================================================================

#include "neoflux/media/media_player.h"
#include "neoflux/media/mpv_media_player.h"

#include <glog/logging.h>

namespace neoflux {

std::unique_ptr<MediaPlayer> CreateMediaPlayer() {
#ifdef NEOFLUX_PLATFORM_DESKTOP
  // Desktop: libmpv with OpenGL texture output (GL backend module only;
  // no-op stub unless NEOFLUX_HAS_MPV is defined by the build).
  return std::make_unique<MpvMediaPlayer>();
#else
  // KNOWN LIMITATION (mobile): no player backend exists on mobile yet. The
  // native backends (Android MediaPlayer/ExoPlayer, iOS AVPlayer) require
  // JNI/ObjC integration that has not been built; returning nullptr makes
  // MediaWidget show its "No media loaded" placeholder, which is the honest
  // degraded state. Logged so the gap is visible, not silent.
  LOG(WARNING) << "CreateMediaPlayer: mobile media playback is not "
                  "implemented yet (no JNI/ObjC player backend); returning "
                  "nullptr -- MediaWidget will show its placeholder.";
  return nullptr;
#endif
}

}  // namespace neoflux
