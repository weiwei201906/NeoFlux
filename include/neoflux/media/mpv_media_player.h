// =============================================================================
// NeoFlux - mpv_media_player.h
//
// libmpv-based media player implementation for desktop platforms.
// Uses the mpv render API to decode video frames into an OpenGL texture that
// can be composited by the NeoFlux render layer.
//
// All method implementations are in src/media/mpv_media_player.cpp.
// =============================================================================

#ifndef NEOFLUX_MEDIA_MPV_MEDIA_PLAYER_H_
#define NEOFLUX_MEDIA_MPV_MEDIA_PLAYER_H_

#include <cstdint>
#include <mutex>
#include <string>

#include "neoflux/media/media_player.h"

// Forward declarations to avoid including mpv headers in the public API.
typedef struct mpv_handle mpv_handle;
typedef struct mpv_render_context mpv_render_context;

namespace neoflux {

// libmpv-backed media player. Decodes video via libmpv and outputs frames to
// an OpenGL texture. Must be initialized on the render thread (InitRender)
// before playback starts.
class MpvMediaPlayer final : public MediaPlayer {
 public:
  MpvMediaPlayer();
  ~MpvMediaPlayer() override;

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
  // Creates the mpv handle and configures basic options. Called from the
  // constructor. Returns true on success.
  bool CreateMpvHandle();

  // mpv event handler. Polls events and updates state/properties.
  void PollEvents();

  // Sets an mpv property as double. Thread-safe.
  void SetPropertyDouble(const char* name, double value);

  // Gets an mpv property as double. Returns 0 on failure.
  [[nodiscard]] double GetPropertyDouble(const char* name) const;

  // Sends a command to mpv (e.g. "loadfile", "pause").
  void Command(const char* args[]);

  mpv_handle* mpv_ = nullptr;
  mpv_render_context* render_ctx_ = nullptr;
  std::uint32_t texture_id_ = 0;
  int video_width_ = 0;
  int video_height_ = 0;
  double volume_ = 1.0;
  MediaState state_ = MediaState::kIdle;
  std::string source_{};
  StateCallback state_callback_{};
  FrameCallback frame_callback_{};
  mutable std::mutex mutex_{};
  bool render_initialized_ = false;
};

}  // namespace neoflux

#endif  // NEOFLUX_MEDIA_MPV_MEDIA_PLAYER_H_
