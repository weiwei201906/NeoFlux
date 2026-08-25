// =============================================================================
// NeoFlux - media_player_factory.cpp
//
// Factory function that creates the platform-appropriate MediaPlayer.
// =============================================================================

#include "neoflux/media/media_player.h"
#include "neoflux/media/mpv_media_player.h"

namespace neoflux {

std::unique_ptr<MediaPlayer> CreateMediaPlayer() {
#ifdef NEOFLUX_PLATFORM_DESKTOP
  // Desktop: libmpv with OpenGL texture output.
  return std::make_unique<MpvMediaPlayer>();
#else
  // Mobile: native player backends (Android MediaPlayer/ExoPlayer,
  // iOS AVPlayer). These require platform-specific JNI/ObjC integration
  // and are instantiated via the mobile bridge.
  return nullptr;
#endif
}

}  // namespace neoflux
