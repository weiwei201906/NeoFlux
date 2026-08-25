// =============================================================================
// NeoFlux - android_media_player.h
//
// Android media player implementation. Uses the Android MediaPlayer API via
// JNI to decode video, and a SurfaceTexture to output frames to an OpenGL ES
// texture that the NeoFlux render layer can composite.
//
// This file is compiled only on Android (ANDROID macro defined by the NDK).
// All method implementations are in src/media/android_media_player.cpp.
// =============================================================================

#ifndef NEOFLUX_MEDIA_ANDROID_MEDIA_PLAYER_H_
#define NEOFLUX_MEDIA_ANDROID_MEDIA_PLAYER_H_

#ifdef ANDROID

#include <cstdint>
#include <mutex>
#include <string>

#include <jni.h>

#include "neoflux/media/media_player.h"

namespace neoflux {

// Android media player backed by android.media.MediaPlayer + SurfaceTexture.
//
// Lifecycle:
//   1. SetSource() stores the URI/path
//   2. InitRender() creates the SurfaceTexture + GL texture on the render thread
//   3. Play() creates the MediaPlayer and attaches the SurfaceTexture
//   4. UpdateTexture() updates the SurfaceTexture and returns the GL texture
//   5. Stop()/Pause() control playback
//
// The MediaPlayer runs on a separate thread (managed by the Android runtime);
// this class only issues JNI calls and polls for state changes.
class AndroidMediaPlayer final : public MediaPlayer {
 public:
  AndroidMediaPlayer();
  ~AndroidMediaPlayer() override;

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

  // Called from JNI when the MediaPlayer preparation completes.
  void OnPrepared(int width, int height);

  // Called from JNI when playback completes.
  void OnCompletion();

  // Called from JNI when an error occurs.
  void OnError(int what, int extra);

  // Sets the JavaVM pointer (called from JNI_OnLoad).
  static void SetJavaVm(JavaVM* vm) noexcept;

 private:
  // Initializes JNI method IDs for MediaPlayer and SurfaceTexture.
  // Must be called from a thread attached to the JVM.
  bool InitJni();

  // Creates the SurfaceTexture and underlying GL texture.
  // Called on the render thread (InitRender).
  bool CreateSurfaceTexture();

  // Releases the MediaPlayer and SurfaceTexture resources.
  void Release();

  // JNI environment helpers.
  [[nodiscard]] JNIEnv* GetEnv() const;
  void DetachThread() const;

  // JavaVM pointer (set at construction via JNI_OnLoad or passed in).
  static JavaVM* java_vm_;

  // JNI class/method references (cached for performance).
  jclass media_player_class_ = nullptr;
  jclass surface_texture_class_ = nullptr;
  jobject media_player_ = nullptr;
  jobject surface_texture_ = nullptr;
  jobject surface_ = nullptr;

  std::uint32_t texture_id_ = 0;
  int video_width_ = 0;
  int video_height_ = 0;
  double volume_ = 1.0;
  MediaState state_ = MediaState::kIdle;
  std::string source_{};
  StateCallback state_callback_{};
  FrameCallback frame_callback_{};
  mutable std::mutex mutex_{};
  bool jni_initialized_ = false;
  bool render_initialized_ = false;
};

}  // namespace neoflux

#endif  // ANDROID

#endif  // NEOFLUX_MEDIA_ANDROID_MEDIA_PLAYER_H_
