// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - media_player_factory.cpp
//
// Factory function that creates the platform-appropriate MediaPlayer.
//
// ARCHITECTURE: the media backend is backend-agnostic. libmpv decodes frames
// through its SOFTWARE render API (MPV_RENDER_API_TYPE_SW) into a CPU buffer,
// the buffer is published as a tgfx image under an opaque frame-image id, and
// TgfxRenderer composites it with Canvas::drawImageRect() -- a path that works
// on every tgfx backend (OpenGL, Metal, Vulkan, D3D12). The build therefore
// defines NEOFLUX_HAS_MPV purely on libmpv availability, with no dependency on
// which tgfx backend is selected (see neoflux/CMakeLists.txt).
// When libmpv is absent:
//   - MpvMediaPlayer compiles as a no-op stub (UpdateFrame() returns 0);
//   - MediaWidget::Paint() draws its "No media loaded" placeholder.
// The MediaPlayer interface and the kDrawTexture protocol command stay
// backend-agnostic on purpose: producers only ever hand over opaque image ids.
// =============================================================================

#include "neoflux/media/media_player.h"
#include "neoflux/media/mpv_media_player.h"

#include <glog/logging.h>

namespace neoflux {

std::unique_ptr<MediaPlayer> CreateMediaPlayer() {
#ifdef NEOFLUX_PLATFORM_DESKTOP
  // Desktop: libmpv with software-rendered CPU frame output (no-op stub unless
  // NEOFLUX_HAS_MPV is defined by the build).
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
