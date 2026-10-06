// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - media_player.h
//
// Abstract media player interface. Decouples widget layer from platform-
// specific playback backends (libmpv on desktop, native players on mobile).
//
// Implementations decode video frames on the CPU and publish them as opaque
// frame-image ids that the render layer resolves and composites into the
// widget tree. NeoFlux owns no GPU objects of its own: the frame image is
// handed to tgfx, which uploads it through whichever GPU backend is active.
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
// |image_id| is the opaque frame-image id the backend published (0 = no frame
// yet); it is NOT a GPU handle and must not be interpreted as one. |width| and
// |height| are the pixel dimensions of the published frame.
using FrameCallback = std::function<void(std::uint32_t image_id, int width,
                                         int height)>;

// Abstract media player. Concrete implementations:
//   - MpvMediaPlayer (desktop): libmpv software render API -> CPU frame image
//   - AndroidMediaPlayer (mobile): MediaPlayer/ExoPlayer -> CPU-frame decode
//   - IosMediaPlayer (mobile): AVPlayer -> CVPixelBuffer -> CPU frame image
//
// The render contract is backend-agnostic: the backend turns decoded frames
// into an opaque image id, and the render layer composites that image through
// tgfx. No implementation may require a current OpenGL context.
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

  // Registers a callback invoked when a new frame has been published.
  virtual void SetFrameCallback(FrameCallback callback) = 0;

  // Registers a non-blocking callback invoked on the decoder's internal thread
  // the moment a new frame is decoded. Backends that decode synchronously or
  // do not need external frame signalling may ignore it (default no-op). The
  // callback must not block and must not touch GPU or driver state; its only
  // purpose is to wake the render thread.
  virtual void SetWakeCallback(const std::function<void()>& callback) {
    (void)callback;
  }

  // Render thread. Initializes the backend's render API (e.g. the mpv render
  // context). No OpenGL/GPU context is required: decoders produce CPU frames.
  virtual void InitRender() = 0;

  // Render thread. Polls the backend for a new frame and publishes it as a
  // CPU image the render layer can composite. Returns the current opaque
  // frame-image id (0 if no frame has been published yet); the id is stable
  // for a given producer and is re-bound to the newest frame on every call,
  // so the returned value may be reused across frames. Never called
  // concurrently with TeardownRender(). Ownership: the id stays owned by the
  // producer, which releases it in TeardownRender(); consumers only resolve
  // it for the duration of one draw.
  [[nodiscard]] virtual std::uint32_t UpdateFrame() = 0;

  // Render thread. MUST be the very LAST render-thread operation on this
  // player, before the App thread destroys it. Shuts the render API down and
  // releases every resource the backend allocated for rendering, including the
  // frame image published by UpdateFrame() (after this the last id resolves to
  // nothing and MUST NOT be drawn again). It must run on the render thread
  // because that is the thread that drives UpdateFrame(): tearing down there
  // serializes the release against an in-flight frame pull. No GPU context is
  // required and none is touched -- the resources are CPU-side. After this
  // returns the player MUST NOT be touched from the render thread again; the
  // App thread may then safely destroy the player (which only tears down the
  // non-render core). Safe to call when the render API was never initialized.
  // The base implementation is a no-op for backends that own no render state.
  virtual void TeardownRender() {}
};

// Factory: creates the platform-appropriate media player.
// Returns nullptr if no backend is available on this platform.
[[nodiscard]] std::unique_ptr<MediaPlayer> CreateMediaPlayer();

}  // namespace neoflux

#endif  // NEOFLUX_MEDIA_MEDIA_PLAYER_H_
