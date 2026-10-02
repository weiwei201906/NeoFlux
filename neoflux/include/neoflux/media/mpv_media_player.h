// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - mpv_media_player.h
//
// libmpv-based media player implementation for desktop platforms.
// Uses the mpv render API to decode video frames into an OpenGL texture that
// can be composited by the NeoFlux render layer.
//
// Pimpl: this header leaks no mpv or GL types. All implementation state and
// the mpv/GL interaction live in MpvMediaPlayer::Impl, defined in the .cpp.
//
// All method implementations are in src/media/mpv_media_player.cpp.
// =============================================================================

#ifndef NEOFLUX_MEDIA_MPV_MEDIA_PLAYER_H_
#define NEOFLUX_MEDIA_MPV_MEDIA_PLAYER_H_

#include <cstdint>
#include <functional>
#include <memory>

#include "neoflux/media/media_player.h"

namespace neoflux {

// libmpv-backed media player. Decodes video via libmpv and outputs frames to
// an OpenGL texture.
//
// ---------------------------------------------------------------------------
// THREADING MODEL
// ---------------------------------------------------------------------------
// This implementation is driven by three distinct threads. The affinity of
// every method below is part of its contract; calling a method from the wrong
// thread is a data race, not a performance issue.
//
//   [App/UI thread]   - the thread running Application's EventLoop. Owns the
//                       widget tree and all widget lifecycle.
//   [Render thread]   - the thread owned by RenderLayer where the OpenGL
//                       context is current. All GL work MUST happen here.
//   [mpv internal]    - an arbitrary thread owned by libmpv. The render-update
//                       callback fires here whenever a new decoded frame is
//                       available.
//
// App/UI-thread methods (safe to call from the EventLoop only; never touch GL):
//   SetSource(), GetSource(), Play(), Pause(), Stop(), Seek(), SetVolume(),
//   GetVolume(), SetStateCallback(), SetFrameCallback(), SetWakeCallback().
//
// Render-thread methods (require a CURRENT OpenGL context; never call from the
// App/UI thread):
//   InitRender(), UpdateTexture().
//
// Thread-safe / any-thread:
//   GetState(), GetVideoWidth(), GetVideoHeight(), GetPosition(),
//   GetDuration(), GetRenderUpdateCount(). These read only atomic or
//   internally-locked state and may be polled from any thread.
//
// Callbacks:
//   StateCallback  - fires on whichever thread pumped mpv events. In the
//                    framework that is the render thread (UpdateTexture calls
//                    PollEvents), but Play()/Pause()/Stop() may also dispatch
//                    it synchronously on the App thread. Treat it as a generic
//                    callback: do NOT block, do NOT touch GL, and NEVER call
//                    SetStateCallback/SetFrameCallback/SetWakeCallback from
//                    inside a callback (the swap would race the invocation).
//   FrameCallback  - fires on the render thread, synchronously at the end of
//                    UpdateTexture(), with the GL context current. It hands off
//                    the freshly composited texture_id. Same re-entrancy rule.
//   WakeCallback   - fires on the mpv internal thread the instant a new frame
//                    is decoded. It MUST be non-blocking and MUST NOT touch GL;
//                    its sole purpose is to wake the render thread (e.g. call
//                    RenderLayer::Wake()). It never runs GL commands.
//
// The source string returned by GetSource() is owned by this object and is
// only mutated by SetSource(). Both are App/UI-thread affine, so the returned
// view is valid as long as the caller does not call SetSource() concurrently
// (which would reallocate the string). Do not read it from the render thread
// while the App thread may be calling SetSource().
// ---------------------------------------------------------------------------
class MpvMediaPlayer final : public MediaPlayer {
 public:
  MpvMediaPlayer();
  ~MpvMediaPlayer() override;

  // --- MediaPlayer interface ---
  // App/UI thread.
  void SetSource(std::string_view source) override;
  // App/UI thread. Returns a view into the internally-owned source string; see
  // the threading-model note above for its lifetime contract.
  [[nodiscard]] std::string_view GetSource() const noexcept override;
  // App/UI thread.
  void Play() override;
  // App/UI thread.
  void Pause() override;
  // App/UI thread.
  void Stop() override;
  // App/UI thread.
  void Seek(double position_seconds) override;
  // App/UI thread.
  void SetVolume(double volume) override;
  // Any thread.
  [[nodiscard]] double GetVolume() const noexcept override;
  // Any thread.
  [[nodiscard]] double GetPosition() const noexcept override;
  // Any thread.
  [[nodiscard]] double GetDuration() const noexcept override;
  // Any thread (atomic read).
  [[nodiscard]] MediaState GetState() const noexcept override;
  // Any thread.
  [[nodiscard]] int GetVideoWidth() const noexcept override;
  // Any thread.
  [[nodiscard]] int GetVideoHeight() const noexcept override;

  // Number of times the libmpv render update callback has fired since the
  // render context was created. The callback runs on an internal mpv thread;
  // this counter is atomic and safe to poll from any thread. Primarily exposed
  // for observability and tests (e.g. verifying the GPU render path actually
  // receives frame-update notifications).
  [[nodiscard]] std::uint32_t GetRenderUpdateCount() const noexcept;

  // App/UI thread. Registers the state-change callback. Never call this from
  // inside a StateCallback/FrameCallback/WakeCallback.
  void SetStateCallback(StateCallback callback) override;
  // App/UI thread. Registers the per-frame callback (fires on the render
  // thread). Never call this from inside a callback.
  void SetFrameCallback(FrameCallback callback) override;

  // Registers the frame-available wake callback. Fires on the mpv internal
  // thread the moment a new decoded frame is ready. Must be non-blocking and
  // must not touch GL; its only job is to wake the render thread (e.g.
  // RenderLayer::Wake()). Set once at wiring time; never swap from inside a
  // callback.
  void SetWakeCallback(std::function<void()> callback);

  // Render thread. Must be called with a current OpenGL context. Creates the
  // mpv render context and installs the update callback.
  void InitRender() override;
  // Render thread. Must be called with a current OpenGL context. Pulls the next
  // mpv event, and if a new frame is available composites it into the cached
  // FBO-backed texture. Returns the current GL texture name (0 until the first
  // frame is decoded). Cheap to call when no new frame is ready.
  [[nodiscard]] std::uint32_t UpdateTexture() override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  // C-callable trampoline for mpv_render_context_set_update_callback. ctx is
  // this object; it runs on an arbitrary mpv internal thread. It only bumps
  // the atomic observability counter, raises the new-frame flag, and invokes
  // the (non-blocking) wake callback. It NEVER touches GL and never blocks.
  // Static so it converts to a plain function pointer without leaking mpv/GL
  // types into this header.
  static void OnRenderUpdate(void* ctx);
};

}  // namespace neoflux

#endif  // NEOFLUX_MEDIA_MPV_MEDIA_PLAYER_H_
