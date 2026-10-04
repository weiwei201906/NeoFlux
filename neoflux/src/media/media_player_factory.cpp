// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - media_player_factory.cpp
//
// Factory function that creates the platform-appropriate MediaPlayer.
//
// ARCHITECTURE: media playback is bound to the OpenGL backend module. libmpv
// decodes frames into GL textures and TgfxRenderer composites them via tgfx's
// GL texture interop (kDrawTexture) -- a path that exists only on the gl
// backend. The build therefore defines NEOFLUX_HAS_MPV only when
// NEOFLUX_BACKEND=gl (see neoflux/CMakeLists.txt). On any other backend:
//   - mpv is not probed, downloaded, or linked;
//   - MpvMediaPlayer compiles as a no-op stub (UpdateTexture() returns 0);
//   - MediaWidget::Paint() draws its "No media loaded" placeholder.
// The MediaPlayer interface and the kDrawTexture protocol command stay
// backend-agnostic on purpose: a future backend can implement its own texture
// importer without touching this module or the render protocol.
// =============================================================================

#include "neoflux/media/media_player.h"
#include "neoflux/media/mpv_media_player.h"

namespace neoflux {

std::unique_ptr<MediaPlayer> CreateMediaPlayer() {
#ifdef NEOFLUX_PLATFORM_DESKTOP
  // Desktop: libmpv with OpenGL texture output (GL backend module only;
  // no-op stub unless NEOFLUX_HAS_MPV is defined by the build).
  return std::make_unique<MpvMediaPlayer>();
#else
  // Mobile: native player backends (Android MediaPlayer/ExoPlayer,
  // iOS AVPlayer). These require platform-specific JNI/ObjC integration
  // and are instantiated via the mobile bridge.
  return nullptr;
#endif
}

}  // namespace neoflux
