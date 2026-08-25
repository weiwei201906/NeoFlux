// =============================================================================
// NeoFlux - ios_media_player.h
//
// iOS media player implementation. Uses AVPlayer to decode video and outputs
// frames to a CVOpenGLESTexture that the NeoFlux render layer composites.
//
// This file is compiled only on iOS (TARGET_OS_IPHONE defined).
// Implementation is in ios_media_player.mm (Objective-C++).
// =============================================================================

#ifndef NEOFLUX_MEDIA_IOS_MEDIA_PLAYER_H_
#define NEOFLUX_MEDIA_IOS_MEDIA_PLAYER_H_

#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)

#include <cstdint>
#include <mutex>
#include <string>

#include "neoflux/media/media_player.h"

// Forward declarations to avoid pulling in Objective-C headers in the public API.
@class AVPlayer;
@class AVPlayerItem;
@class AVPlayerLayer;
@class CADisplayLink;
@class CVPixelBuffer;
@class EAGLContext;

namespace neoflux {

// iOS media player backed by AVPlayer + CVPixelBuffer + CVOpenGLESTexture.
//
// Video frames are delivered via AVPlayerItemVideoOutput, converted to an
// OpenGL ES texture via CVOpenGLESTextureCache, and composited by the render
// layer. Audio is played automatically by AVPlayer.
class IosMediaPlayer final : public MediaPlayer {
 public:
  IosMediaPlayer();
  ~IosMediaPlayer() override;

  // --- MediaPlayer interface ---
  void SetSource(std::string_view source) override;
  [[nodiscard]] std::string_view GetSource() const noexcept override;
  void Play() override;
  void Pause() override;
  void Stop() override;
  void Seek(double position_seconds) override;
  void SetVolume(double volume) override;
  [[nodiscard]] double GetVolume() const noexcept override;
  [[nodiscard]] double GetPosition() const noexcept override;
  [[nodiscard]] double GetDuration() const noexcept override;
  [[nodiscard]] MediaState GetState() const noexcept override;
  [[nodiscard]] int GetVideoWidth() const noexcept override;
  [[nodiscard]] int GetVideoHeight() const noexcept override;
  void SetStateCallback(StateCallback callback) override;
  void SetFrameCallback(FrameCallback callback) override;
  void InitRender() override;
  [[nodiscard]] std::uint32_t UpdateTexture() override;

 private:
  // Creates the CVOpenGLESTextureCache for converting CVPixelBuffers to GL
  // textures. Must be called on the render thread with a current EAGL context.
  bool CreateTextureCache();

  // Polls AVPlayerItemVideoOutput for the latest pixel buffer and updates the
  // GL texture. Returns the texture name (0 if no new frame).
  std::uint32_t UpdateTextureFromPixelBuffer();

  // Objective-C state (opaque pointers to avoid ObjC in header).
  void* player_ = nullptr;        // AVPlayer*
  void* player_item_ = nullptr;   // AVPlayerItem*
  void* video_output_ = nullptr;  // AVPlayerItemVideoOutput*
  void* texture_cache_ = nullptr; // CVOpenGLESTextureCacheRef
  void* display_link_ = nullptr;  // CADisplayLink*

  std::uint32_t texture_id_ = 0;
  int texture_width_ = 0;
  int texture_height_ = 0;
  double volume_ = 1.0;
  MediaState state_ = MediaState::kIdle;
  std::string source_{};
  StateCallback state_callback_{};
  FrameCallback frame_callback_{};
  mutable std::mutex mutex_{};
  bool render_initialized_ = false;
};

}  // namespace neoflux

#endif  // __APPLE__ && TARGET_OS_IPHONE

#endif  // NEOFLUX_MEDIA_IOS_MEDIA_PLAYER_H_
