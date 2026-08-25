// =============================================================================
// NeoFlux - media_player_factory.cpp
//
// Factory function that creates the platform-appropriate MediaPlayer.
//
//   Desktop (Windows/Linux/macOS): MpvMediaPlayer (libmpv + OpenGL)
//   Android:                         AndroidMediaPlayer (JNI + MediaPlayer)
//   iOS:                             IosMediaPlayer (AVPlayer + CVPixelBuffer)
// =============================================================================

#include "neoflux/media/media_player.h"
#include "neoflux/media/desktop/mpv_media_player.h"

#ifdef ANDROID
#include "neoflux/media/android/android_media_player.h"
#endif

#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)
#include "neoflux/media/ios/ios_media_player.h"
#endif

namespace neoflux {

std::unique_ptr<MediaPlayer> CreateMediaPlayer() {
#ifdef ANDROID
  return std::make_unique<AndroidMediaPlayer>();
#elif defined(__APPLE__) && defined(TARGET_OS_IPHONE)
  return std::make_unique<IosMediaPlayer>();
#else
  // Desktop: libmpv with OpenGL texture output.
  return std::make_unique<MpvMediaPlayer>();
#endif
}

}  // namespace neoflux
