// =============================================================================
// NeoFlux - media_player.h
//
// Abstract media player interface. Decouples widget layer from platform-
// specific playback backends (libmpv on desktop, native players on mobile).
//
// Implementations decode video frames to an OpenGL texture that the render
// layer can composite into the widget tree (Flutter-style texture sharing).
//
// All method implementations are in src/media/.
// =============================================================================

#ifndef NEOFLUX_MEDIA_MEDIA_PLAYER_H_
#define NEOFLUX_MEDIA_MEDIA_PLAYER_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace neoflux {

// Playback state of a media player.
enum class MediaState : std::uint8_t {
  kIdle = 0,     // No source loaded.
  kLoading = 1,  // Source is being loaded / buffered.
  kPlaying = 2,  // Playback is active.
  kPaused = 3,   // Playback is paused.
  kEnded = 4,    // Playback reached end of stream.
  kError = 5,    // An error occurred.
};

// Callback invoked when the player state changes.
using StateCallback = std::function<void(MediaState state)>;

// Callback invoked when a new video frame is ready for compositing.
// The texture_id is an OpenGL texture name (0 = no frame yet).
using FrameCallback = std::function<void(std::uint32_t texture_id, int width,
                                         int height)>;

// Abstract media player. Concrete implementations:
//   - MpvMediaPlayer (desktop): libmpv render API -> OpenGL texture
//   - AndroidMediaPlayer (mobile): MediaPlayer/ExoPlayer -> SurfaceTexture
//   - IosMediaPlayer (mobile): AVPlayer -> CVPixelBuffer -> CVOpenGLESTexture
class MediaPlayer {
 public:
  virtual ~MediaPlayer() = default;

  // Sets the media source (file path or URL). Must be called before Play().
  virtual void SetSource(std::string_view source) = 0;

  // Returns the current source, or empty if none set.
  [[nodiscard]] virtual std::string_view GetSource() const noexcept = 0;

  // Starts playback. If paused, resumes from current position.
  virtual void Play() = 0;

  // Pauses playback at the current position.
  virtual void Pause() = 0;

  // Stops playback and resets position to 0.
  virtual void Stop() = 0;

  // Seeks to the given position in seconds.
  virtual void Seek(double position_seconds) = 0;

  // Sets the playback volume (0.0 = mute, 1.0 = full).
  virtual void SetVolume(double volume) = 0;

  // Returns the current volume.
  [[nodiscard]] virtual double GetVolume() const noexcept = 0;

  // Returns the current playback position in seconds.
  [[nodiscard]] virtual double GetPosition() const noexcept = 0;

  // Returns the total duration in seconds, or 0 if unknown.
  [[nodiscard]] virtual double GetDuration() const noexcept = 0;

  // Returns the current playback state.
  [[nodiscard]] virtual MediaState GetState() const noexcept = 0;

  // Returns the video width in pixels, or 0 if no video track.
  [[nodiscard]] virtual int GetVideoWidth() const noexcept = 0;

  // Returns the video height in pixels, or 0 if no video track.
  [[nodiscard]] virtual int GetVideoHeight() const noexcept = 0;

  // Registers a callback invoked on state transitions.
  virtual void SetStateCallback(StateCallback callback) = 0;

  // Registers a callback invoked when a new frame texture is available.
  virtual void SetFrameCallback(FrameCallback callback) = 0;

  // Must be called from the render thread with a current GL context.
  // Initializes the render API (e.g. mpv_render_context).
  virtual void InitRender() = 0;

  // Called each frame from the render thread. Updates the video texture if
  // a new frame is available. Returns the current GL texture name (0 if no
  // frame has been decoded yet).
  [[nodiscard]] virtual std::uint32_t UpdateTexture() = 0;
};

// Factory: creates the platform-appropriate media player.
// Returns nullptr if no backend is available on this platform.
[[nodiscard]] std::unique_ptr<MediaPlayer> CreateMediaPlayer();

}  // namespace neoflux

#endif  // NEOFLUX_MEDIA_MEDIA_PLAYER_H_
