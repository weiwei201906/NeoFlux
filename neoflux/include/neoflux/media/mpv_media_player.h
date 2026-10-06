// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - mpv_media_player.h
//
// libmpv-based media player implementation for desktop platforms.
// Decodes video through the mpv software render API (MPV_RENDER_API_TYPE_SW)
// into a CPU buffer, wraps that buffer as a tgfx image and publishes it under
// an opaque frame-image id that the NeoFlux render layer composites.
//
// Pimpl: this header leaks no mpv, tgfx or GL types. All implementation state
// and the mpv interaction live in MpvMediaPlayer::Impl, defined in the .cpp.
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

// libmpv-backed media player. Decodes video via libmpv into a CPU frame and
// publishes the resulting tgfx image for the render layer.
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
//   [Render thread]   - the thread owned by RenderLayer. It drives the render
//                       pump, so all frame pulling happens here. It also owns
//                       the GPU context, but this player does not use it.
//   [mpv internal]    - an arbitrary thread owned by libmpv. The render-update
//                       callback fires here whenever a new decoded frame is
//                       available.
//
// App/UI-thread methods (safe to call from the EventLoop only):
//   SetSource(), GetSource(), Play(), Pause(), Stop(), Seek(), SetVolume(),
//   GetVolume(), SetStateCallback(), SetFrameCallback(), SetWakeCallback().
//
// Render-thread methods (never call from the App/UI thread):
//   InitRender(), UpdateFrame(), TeardownRender().
//
//   None of them needs a current OpenGL context, because the mpv software
//   renderer writes into a CPU buffer: there is no GL object to create,
//   upload or delete anywhere in the media path.
//
// DESTRUCTION / TEARDOWN ORDERING (read this before changing ~MpvMediaPlayer):
//   InitRender(), UpdateFrame() and TeardownRender() are all driven by the
//   render thread. TeardownRender() frees the mpv render context and drops the
//   published frame image, so it MUST NOT run concurrently with a frame pull;
//   routing it through the render thread (RenderLayer::RunOnRenderThread) is
//   what guarantees that serialization.
//
//   Correct shutdown sequence (driven by MediaWidget):
//     1. [App thread] MediaWidget dtor: SetRenderPump(nullptr), SetWakeCallback,
//        then RenderLayer::RunOnRenderThread([p]{ p->TeardownRender(); }) which
//        BLOCKS the App thread until the render thread has freed the mpv render
//        context and released the frame image.
//     2. [App thread] ~MpvMediaPlayer/~Impl then only calls mpv_terminate_destroy
//        (mpv core teardown). render_ctx is already nullptr.
//
//   If TeardownRender() was never invoked (e.g. unit tests that own the player
//   on a single thread, or a player whose render context was never created),
//   ~Impl frees the remaining render state inline. That is safe on any thread
//   because the state is CPU-only -- but it is still only race-free because no
//   frame pull can be in flight once the player is being destroyed.
//
// Thread-safe / any-thread:
//   GetState(), GetVideoWidth(), GetVideoHeight(), GetPosition(),
//   GetDuration(), GetRenderUpdateCount(). These read only atomic or
//   internally-locked state and may be polled from any thread.
//
// Callbacks:
//   StateCallback  - fires on whichever thread pumped mpv events. In the
//                    framework that is the render thread (UpdateFrame calls
//                    PollEvents), but Play()/Pause()/Stop() may also dispatch
//                    it synchronously on the App thread. Treat it as a generic
//                    callback: do NOT block and NEVER call
//                    SetStateCallback/SetFrameCallback/SetWakeCallback from
//                    inside a callback (the swap would race the invocation).
//   FrameCallback  - fires on the render thread, synchronously at the end of
//                    UpdateFrame(), and hands off the freshly published frame
//                    image id plus its pixel dimensions. Same re-entrancy rule.
//   WakeCallback   - fires on the mpv internal thread the instant a new frame
//                    is decoded. It MUST be non-blocking and MUST NOT touch
//                    GPU/driver state; its sole purpose is to wake the render
//                    thread (e.g. call RenderLayer::Wake()).
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
  // for observability and tests (e.g. verifying the render path actually
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
  // must not touch GPU/driver state; its only job is to wake the render thread
  // (e.g. RenderLayer::Wake()). Set once at wiring time; never swap from
  // inside a callback.
  void SetWakeCallback(const std::function<void()>& callback) override;

  // Render thread. Creates the mpv render context with MPV_RENDER_API_TYPE_SW
  // and installs the update callback. No OpenGL context is required. No-op if
  // the render context already exists or the mpv handle could not be created.
  void InitRender() override;

  // Render thread. Polls the mpv event queue and, when mpv reports a new
  // frame, renders it into the CPU frame buffer and republishes it as the
  // player's frame image. Returns the current frame-image id (0 until the
  // first frame has been rendered); the same id is returned for every
  // subsequent frame, re-bound to the newest image. Cheap to call when no new
  // frame is ready. Must not run concurrently with TeardownRender().
  [[nodiscard]] std::uint32_t UpdateFrame() override;

  // Render thread. Last render-thread operation: detaches the mpv update
  // callback, frees the mpv render context, releases the published frame
  // image and drops the CPU frame buffers. Block the App thread on this (via
  // RenderLayer::RunOnRenderThread) BEFORE destroying the player, so it cannot
  // race a frame pull. Safe to call when the render context was never created
  // (no-op).
  void TeardownRender() override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  // C-callable trampoline for mpv_render_context_set_update_callback. ctx is
  // this object; it runs on an arbitrary mpv internal thread. It only bumps
  // the atomic observability counter, raises the new-frame flag, and invokes
  // the (non-blocking) wake callback. It NEVER touches GPU/driver state and
  // never blocks. Static so it converts to a plain function pointer without
  // leaking mpv types into this header.
  static void OnRenderUpdate(void* ctx);
};

}  // namespace neoflux

#endif  // NEOFLUX_MEDIA_MPV_MEDIA_PLAYER_H_
