// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - mpv_media_player.cpp
//
// libmpv media player implementation. Uses the mpv SOFTWARE render API
// (MPV_RENDER_API_TYPE_SW) to decode video frames into a CPU buffer, converts
// that buffer into a tgfx::Image and publishes it under an opaque frame-image
// id that the render layer composites. No OpenGL call and no GL header is
// involved: the whole mpv -> screen path is backend-agnostic and works
// unchanged on tgfx's OpenGL, Metal, Vulkan and D3D12 backends.
//
// Pimpl: MpvMediaPlayer::Impl (defined here) owns all mpv and frame-buffer
// state, so neither mpv nor tgfx headers leak into mpv_media_player.h.
//
// Threading: see the "THREADING MODEL" block in mpv_media_player.h. In short:
//   - App/UI thread  : Set/GetSource, Play, Pause, Stop, Seek, SetVolume,
//                      SetStateCallback, SetFrameCallback, SetWakeCallback.
//   - Render thread  : InitRender, UpdateFrame, TeardownRender.
//   - mpv internal   : OnRenderUpdate (frame-available signal, no GPU work).
// All callback std::function objects are guarded by |mutex|; a callback is
// always invoked via a lock-protected local copy so a concurrent swap cannot
// destroy the target mid-call. |state| is an atomic so GetState() needs no
// lock and never races the writers.
// =============================================================================

#include "neoflux/media/mpv_media_player.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <utility>

#ifdef NEOFLUX_HAS_MPV

#include <mpv/client.h>
#include <mpv/render.h>

#include <glog/logging.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include "renderers/frame_image_registry.h"

#include "tgfx/core/AlphaType.h"
#include "tgfx/core/Bitmap.h"
#include "tgfx/core/ColorType.h"
#include "tgfx/core/Image.h"
#include "tgfx/core/ImageInfo.h"

namespace neoflux {

namespace {

// Requested mpv software-render surface layout. Both fourccs are 4 bytes per
// pixel with R at offset 0 and a pixel alignment of 4 bytes:
//   "rgba" - R, G, B, A (unpremultiplied); the internal name that carries a
//            real alpha channel, so transparency in the source is preserved.
//            render.h lists it under "other" internal names, i.e. accepted only
//            if mpv supports it as a conversion output (see the fallback in
//            UpdateFrame).
//   "rgb0" - R, G, B, unused; explicitly documented by render.h as valid, but
//            the fourth byte is undefined garbage, so the frame is declared
//            opaque to tgfx (AlphaType::Opaque).
// The two are paired with the AlphaType used in UpdateFrame; keep them in sync.
constexpr int kSwBytesPerPixel = 4;

// mpv's render.h asks for a target pointer and stride that are multiples of 64
// bytes ("to facilitate fast SIMD operation"); a misaligned target can make mpv
// copy the whole frame internally. The frame buffer below is therefore aligned
// to this boundary, which also satisfies the mandatory multiple-of-pixel-size
// rule.
constexpr std::size_t kSwStrideAlignment = 64;

// Frame size used until mpv reports the real video dimensions.
constexpr int kFallbackFrameWidth = 640;
constexpr int kFallbackFrameHeight = 360;

// Rounds |value| up to the next multiple of |alignment| (a power of two).
// |alignment| MUST be a power of two (enforced by construction above).
constexpr std::size_t AlignUp(std::size_t value, std::size_t alignment) {
  return (value + alignment - 1U) & ~(alignment - 1U);
}

}  // namespace

// =============================================================================
// MpvMediaPlayer::Impl - all mpv state and private operations live here.
// =============================================================================
struct MpvMediaPlayer::Impl {
  Impl() = default;
  ~Impl() {
    // Detach all user callbacks first so none fires while we tear down mpv.
    {
      std::scoped_lock lock(mutex);
      wake_callback = nullptr;
      state_callback = nullptr;
      frame_callback = nullptr;
    }

    // Render-state teardown. In the framework, MediaWidget already ran
    // TeardownRender() on the render thread BEFORE this destructor runs on the
    // App thread, so |render_ctx| is nullptr here and there is nothing left to
    // free. If TeardownRender() was never called (unit tests that own the
    // player on one thread, or a player whose render context was never
    // created), free inline. Everything below is CPU-only (a CPU buffer and a
    // registry entry), so this is safe on any thread; it is race-free because
    // no frame pull can be in flight while the object is being destroyed.
    if (render_ctx != nullptr || frame_image_id != 0) {
      FreeRenderContext();
    }

    // mpv core teardown. Safe on any thread AFTER the render context has been
    // freed (above). Blocks until mpv's internal threads have joined.
    if (mpv != nullptr) {
      mpv_terminate_destroy(mpv);
      mpv = nullptr;
    }
  }

  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(Impl&&) = delete;

  // Frees the mpv render context, releases the published frame image and drops
  // the CPU frame buffers. Render thread only: mpv_render_context_free() must
  // not race UpdateFrame(). Idempotent: safe to call when nothing was
  // allocated. Invoked either by TeardownRender() (framework, render thread) or
  // inline from ~Impl() (tests / never-initialized render context).
  void FreeRenderContext() {
    if (render_ctx != nullptr) {
      mpv_render_context_set_update_callback(render_ctx, nullptr, nullptr);
      mpv_render_context_free(render_ctx);
      render_ctx = nullptr;
    }
    if (frame_image_id != 0) {
      // Drops the registry's reference to the last frame; from now on the id
      // resolves to nothing and the renderer skips it.
      FrameImageRegistry::Release(frame_image_id);
      frame_image_id = 0;
    }
    // Release the frame memory (the vector swap frees the buffer immediately,
    // unlike clear()).
    pixel_storage_ = std::vector<std::uint8_t>();
    pixels = nullptr;
    stride = 0;
    frame_bitmap = tgfx::Bitmap();
    allocated_w = 0;
    allocated_h = 0;
    render_initialized = false;
  }

  // (Re)allocates the CPU frame buffer and the tgfx raster bitmap for a
  // |width| x |height| video frame. Render thread only. Returns false when
  // either allocation fails, in which case |pixels| and |stride| are left
  // cleared and the caller keeps publishing the previous frame. Ownership: the
  // buffer is owned by |pixel_storage_| and stays valid until the next
  // (re)allocation or FreeRenderContext().
  [[nodiscard]] bool AllocateFrameBuffers(int width, int height) {
    const auto row_bytes =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(kSwBytesPerPixel);
    const std::size_t aligned_stride = AlignUp(row_bytes, kSwStrideAlignment);
    const std::size_t frame_bytes =
        aligned_stride * static_cast<std::size_t>(height);

    // Over-allocate by one alignment unit so the base pointer can be advanced
    // to a 64-byte boundary inside the same allocation.
    pixel_storage_.assign(frame_bytes + kSwStrideAlignment, 0);
    void* base = pixel_storage_.data();
    std::size_t space = pixel_storage_.size();
    void* const aligned =
        std::align(kSwStrideAlignment, frame_bytes, base, space);
    if (aligned == nullptr) {
      pixel_storage_ = std::vector<std::uint8_t>();
      pixels = nullptr;
      stride = 0;
      return false;
    }
    pixels = static_cast<std::uint8_t*>(aligned);
    stride = aligned_stride;

    // Raster (never hardware-backed) pixels: mpv writes through the CPU and the
    // frame must be readable by tgfx's raster path on every platform.
    if (!frame_bitmap.allocPixels(width, height, false, false)) {
      pixel_storage_ = std::vector<std::uint8_t>();
      pixels = nullptr;
      stride = 0;
      return false;
    }
    return true;
  }

  // Creates the mpv handle and configures basic options. Returns true on
  // success.
  bool CreateMpvHandle();

  // Sends a command to mpv (e.g. "loadfile", "pause").
  void Command(const char* args[]);

  // Atomically publishes |new_state| and invokes the state callback (if any)
  // via a lock-protected local copy, so a concurrent SetStateCallback swap
  // cannot destroy the std::function mid-call.
  void EmitState(MediaState new_state);

  // mpv event handler. Polls events and updates state/properties. Runs on
  // whichever thread calls UpdateFrame() (the render thread in production).
  void PollEvents();

  // Sets an mpv property as double.
  void SetPropertyDouble(const char* name, double value);

  // Gets an mpv property as double. Returns 0 on failure.
  [[nodiscard]] double GetPropertyDouble(const char* name) const;

  mpv_handle* mpv = nullptr;
  mpv_render_context* render_ctx = nullptr;
  // CPU frame target for the mpv software renderer. |pixel_storage_| owns the
  // memory; |pixels| is its 64-byte aligned base and |stride| the aligned row
  // pitch handed to mpv. Reallocated only when the video size changes.
  std::vector<std::uint8_t> pixel_storage_;
  std::uint8_t* pixels = nullptr;
  std::size_t stride = 0;
  // tgfx raster bitmap that receives the premultiplied copy of the frame. Only
  // the bitmap (not |pixels|) is handed to tgfx, so the published image owns
  // stable pixels even though mpv keeps rewriting the frame buffer.
  tgfx::Bitmap frame_bitmap;
  // Opaque registry id of the published frame (0 = no frame published yet).
  std::uint32_t frame_image_id = 0;
  // Pixel format currently requested from the software renderer. "rgba" is
  // tried first because it preserves a source alpha channel; if mpv rejects it
  // (render.h documents it only as an "other" internal format), UpdateFrame
  // switches to the explicitly documented "rgb0" once and treats frames as
  // opaque. A rejected format is a one-time cost, never a per-frame error loop.
  bool sw_format_rgba = true;
  // Dimensions of the currently allocated frame buffers (0 = unallocated).
  int allocated_w = 0;
  int allocated_h = 0;
  int video_width = 0;
  int video_height = 0;
  double volume = 1.0;
  // Atomic so GetState() (any thread) never races the writers (PollEvents on
  // the render thread; Play/Pause/Stop on the App thread).
  std::atomic<MediaState> state{MediaState::kIdle};
  std::string source;
  StateCallback state_callback;
  FrameCallback frame_callback;
  // Non-blocking wake installed by the framework; invoked on the mpv internal
  // thread when a new frame is decoded. Guarded by |mutex|.
  std::function<void()> wake_callback;
  // Protects |source| and the three std::function callbacks. It is held only
  // for short copies; callbacks are never invoked while held.
  std::mutex mutex;
  bool render_initialized = false;

  // --- Frame-available signalling (mpv internal thread -> render thread) ---
  // OnRenderUpdate raises |new_frame_| under |frame_mutex_| and notifies
  // |frame_cv_|. The render thread consumes the flag in UpdateFrame().
  std::mutex frame_mutex_;
  std::condition_variable frame_cv_;
  std::atomic<bool> new_frame_{false};
  // Incremented by OnRenderUpdate on an arbitrary mpv-internal thread each
  // time a new frame update is signalled. Read from the render thread (or a
  // test) via GetRenderUpdateCount(). Relaxed ordering is sufficient: this is
  // a monotonically increasing observability counter, not used to order data.
  std::atomic<std::uint32_t> update_count_{0};
};

bool MpvMediaPlayer::Impl::CreateMpvHandle() {
  mpv = mpv_create();
  if (mpv == nullptr) {
    LOG(ERROR) << "MpvMediaPlayer: mpv_create failed";
    return false;
  }

  // Configure mpv for embedded rendering: no window, video output via render
  // API.
  mpv_set_option_string(mpv, "no-video", "no");
  mpv_set_option_string(mpv, "vo", "libmpv");
  mpv_set_option_string(mpv, "terminal", "no");
  mpv_set_option_string(mpv, "ytdl", "no");

  const int ret = mpv_initialize(mpv);
  if (ret < 0) {
    LOG(ERROR) << "MpvMediaPlayer: mpv_initialize failed: " << mpv_error_string(ret);
    mpv_terminate_destroy(mpv);
    mpv = nullptr;
    return false;
  }

  // Route mpv's internal log through glog. terminal=no means mpv writes nothing
  // to stderr directly; instead records are delivered as MPV_EVENT_LOG_MESSAGE
  // (consumed in PollEvents). This MUST be after mpv_initialize: it is a command,
  // not an option. "warn" only surfaces real problems; verbose filter-graph
  // teardown chatter is suppressed.
  mpv_request_log_messages(mpv, "warn");

  return true;
}

void MpvMediaPlayer::Impl::SetPropertyDouble(const char* name, double value) {
  if (mpv == nullptr) {
    return;
  }
  mpv_set_property(mpv, name, MPV_FORMAT_DOUBLE, &value);
}

double MpvMediaPlayer::Impl::GetPropertyDouble(const char* name) const {
  if (mpv == nullptr) {
    return 0.0;
  }
  double value = 0.0;
  mpv_get_property(mpv, name, MPV_FORMAT_DOUBLE, &value);
  return value;
}

void MpvMediaPlayer::Impl::Command(const char* args[]) {
  if (mpv == nullptr) {
    return;
  }
  mpv_command_async(mpv, 0, args);
}

void MpvMediaPlayer::Impl::EmitState(MediaState new_state) {
  state.store(new_state);
  // Copy the callback under the lock, then release before invoking so a
  // concurrent SetStateCallback swap cannot destroy the target mid-call.
  StateCallback cb;
  {
    std::scoped_lock lock(mutex);
    cb = state_callback;
  }
  if (cb != nullptr) {
    cb(new_state);
  }
}

void MpvMediaPlayer::Impl::PollEvents() {
  if (mpv == nullptr) {
    return;
  }
  while (true) {
    mpv_event* event = mpv_wait_event(mpv, 0);
    if (event == nullptr || event->event_id == MPV_EVENT_NONE) {
      break;
    }
    switch (event->event_id) {
      case MPV_EVENT_FILE_LOADED:
        LOG(INFO) << "MpvMediaPlayer: file loaded, starting playback";
        SetPropertyDouble("pause", 0.0);
        EmitState(MediaState::kPlaying);
        break;
      case MPV_EVENT_START_FILE:
        LOG(INFO) << "MpvMediaPlayer: starting to load file";
        break;
      case MPV_EVENT_END_FILE: {
        // The structured reason/error lives in event->data, not event->error.
        const auto* end = static_cast<mpv_event_end_file*>(event->data);
        if (end != nullptr && end->reason == MPV_END_FILE_REASON_ERROR) {
          LOG(ERROR) << "MpvMediaPlayer: playback error: "
                     << mpv_error_string(end->error);
          EmitState(MediaState::kError);
        } else {
          LOG(INFO) << "MpvMediaPlayer: end of file (reason="
                    << (end != nullptr ? static_cast<int>(end->reason) : -1) << ")";
          EmitState(MediaState::kEnded);
        }
        break;
      }
      case MPV_EVENT_IDLE:
        state.store(MediaState::kIdle);
        break;
      case MPV_EVENT_LOG_MESSAGE: {
        auto* log = static_cast<mpv_event_log_message*>(event->data);
        if (log == nullptr) {
          break;
        }
        // mpv log->text ends with a newline; strip it so glog does not emit a
        // blank line per record.
        std::string text(log->text != nullptr ? log->text : "");
        while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
          text.pop_back();
        }
        const char* prefix = log->prefix != nullptr ? log->prefix : "mpv";
        const char* level = log->level != nullptr ? log->level : "info";
        // Route mpv severity onto the matching glog level so real problems are
        // not masked as benign INFO chatter.
        if (std::strcmp(level, "error") == 0 || std::strcmp(level, "fatal") == 0) {
          LOG(ERROR) << "[mpv/" << prefix << "] " << text;
        } else if (std::strcmp(level, "warn") == 0) {
          LOG(WARNING) << "[mpv/" << prefix << "] " << text;
        } else {
          LOG(INFO) << "[mpv/" << prefix << "] " << text;
        }
        break;
      }
      case MPV_EVENT_SHUTDOWN:
        // mpv core asked to shut itself down; nothing to do here (the owning
        // Impl destructor handles mpv_terminate_destroy).
        break;
      default:
        break;
    }
  }
}

// =============================================================================
// MpvMediaPlayer - thin forwards to Impl.
// =============================================================================

MpvMediaPlayer::MpvMediaPlayer() : impl_(std::make_unique<Impl>()) {
  if (!impl_->CreateMpvHandle()) {
    impl_->state.store(MediaState::kError);
  }
}

MpvMediaPlayer::~MpvMediaPlayer() = default;

void MpvMediaPlayer::SetSource(std::string_view source) {
  std::scoped_lock lock(impl_->mutex);
  impl_->source = std::string(source);
}

std::string_view MpvMediaPlayer::GetSource() const noexcept {
  // App/UI-thread affine: |source| is only mutated by SetSource() on the same
  // thread, so no lock is needed and the returned view cannot dangle. Do NOT
  // call this concurrently with SetSource() from another thread.
  return impl_->source;
}

void MpvMediaPlayer::Play() {
  if (impl_->mpv == nullptr) {
    LOG(WARNING) << "Play() called but mpv handle is null";
    return;
  }
  // Copy the source under the lock, then issue the command without holding the
  // lock (mpv_command_async must not contend user callbacks).
  std::string source_copy;
  {
    std::scoped_lock lock(impl_->mutex);
    source_copy = impl_->source;
  }
  if (source_copy.empty()) {
    LOG(WARNING) << "Play() called but source is empty";
    return;
  }
  LOG(INFO) << "mpv loadfile: " << source_copy;
  const char* cmd[] = {"loadfile", source_copy.c_str(), nullptr};
  mpv_command_async(impl_->mpv, 0, cmd);
  impl_->EmitState(MediaState::kLoading);
}

void MpvMediaPlayer::Pause() {
  if (impl_->mpv == nullptr) {
    return;
  }
  impl_->SetPropertyDouble("pause", 1.0);
  impl_->EmitState(MediaState::kPaused);
}

void MpvMediaPlayer::Stop() {
  if (impl_->mpv == nullptr) {
    return;
  }
  const char* cmd[] = {"stop", nullptr};
  mpv_command_async(impl_->mpv, 0, cmd);
  impl_->EmitState(MediaState::kIdle);
}

void MpvMediaPlayer::Seek(double position_seconds) {
  if (impl_->mpv == nullptr) {
    return;
  }
  char target[32];
  std::snprintf(target, sizeof(target), "%f", position_seconds);
  const char* cmd[] = {"seek", target, "absolute", nullptr};
  mpv_command_async(impl_->mpv, 0, cmd);
}

void MpvMediaPlayer::SetVolume(double volume) {
  impl_->volume = std::clamp(volume, 0.0, 1.0);
  impl_->SetPropertyDouble("volume", impl_->volume * 100.0);
}

double MpvMediaPlayer::GetVolume() const noexcept {
  return impl_->volume;
}

double MpvMediaPlayer::GetPosition() const noexcept {
  return impl_->GetPropertyDouble("time-pos");
}

double MpvMediaPlayer::GetDuration() const noexcept {
  return impl_->GetPropertyDouble("duration");
}

MediaState MpvMediaPlayer::GetState() const noexcept {
  return impl_->state.load();
}

int MpvMediaPlayer::GetVideoWidth() const noexcept {
  return impl_->video_width;
}

int MpvMediaPlayer::GetVideoHeight() const noexcept {
  return impl_->video_height;
}

std::uint32_t MpvMediaPlayer::GetRenderUpdateCount() const noexcept {
  return impl_->update_count_.load(std::memory_order_relaxed);
}

// Static trampoline for the mpv C update callback. Registered with
// mpv_render_context_set_update_callback in InitRender(); ctx is the
// MpvMediaPlayer* (this). It runs on an arbitrary mpv-internal thread and must
// do as little as possible: bump the observability counter, raise the
// new-frame flag, and invoke the non-blocking wake callback. It MUST NOT touch
// GPU/driver state and MUST NOT block. The actual frame pickup (mpv software
// render into the CPU buffer) happens in UpdateFrame on the render thread.
void MpvMediaPlayer::OnRenderUpdate(void* ctx) {
  auto* const self = static_cast<MpvMediaPlayer*>(ctx);
  if (self == nullptr) {
    return;
  }
  Impl& impl = *self->impl_;
  impl.update_count_.fetch_add(1, std::memory_order_relaxed);
  {
    std::scoped_lock lock(impl.frame_mutex_);
    impl.new_frame_.store(true, std::memory_order_relaxed);
  }
  impl.frame_cv_.notify_one();

  // Invoke the wake callback via a lock-protected local copy. It must be
  // non-blocking and must not touch GPU/driver state.
  std::function<void()> wake;
  {
    std::scoped_lock lock(impl.mutex);
    wake = impl.wake_callback;
  }
  if (wake != nullptr) {
    wake();
  }
}

void MpvMediaPlayer::SetStateCallback(StateCallback callback) {
  std::scoped_lock lock(impl_->mutex);
  impl_->state_callback = std::move(callback);
}

void MpvMediaPlayer::SetFrameCallback(FrameCallback callback) {
  std::scoped_lock lock(impl_->mutex);
  impl_->frame_callback = std::move(callback);
}

void MpvMediaPlayer::SetWakeCallback(const std::function<void()>& callback) {
  std::scoped_lock lock(impl_->mutex);
  impl_->wake_callback = std::move(callback);
}

void MpvMediaPlayer::TeardownRender() {
  // Render thread. Releases the mpv render context, the published frame image
  // and the CPU frame buffers on the thread that drives UpdateFrame(), so the
  // release cannot race an in-flight frame pull. There is no GL object and no
  // GL context involved. FreeRenderContext() is a no-op if nothing was ever
  // allocated.
  impl_->FreeRenderContext();
}

void MpvMediaPlayer::InitRender() {
  if (impl_->mpv == nullptr || impl_->render_initialized) {
    return;
  }

  // The software renderer needs no window system, no GPU device and no entry
  // point loader: only the API type is passed at creation time. mpv's C API
  // types render_param.data as a non-const void*, so make a mutable copy of the
  // API-type macro instead of casting away const.
  char api_type[] = MPV_RENDER_API_TYPE_SW;
  mpv_render_param params[] = {
      {.type = MPV_RENDER_PARAM_API_TYPE, .data = api_type},
      {.type = MPV_RENDER_PARAM_INVALID, .data = nullptr},
  };

  const int ret =
      mpv_render_context_create(&impl_->render_ctx, impl_->mpv, params);
  if (ret < 0) {
    LOG(ERROR) << "MpvMediaPlayer: mpv_render_context_create failed: "
               << mpv_error_string(ret);
    return;
  }

  mpv_render_context_set_update_callback(impl_->render_ctx, &MpvMediaPlayer::OnRenderUpdate,
                                         this);
  impl_->render_initialized = true;
  LOG(INFO) << "MpvMediaPlayer: software render context initialized";
}

std::uint32_t MpvMediaPlayer::UpdateFrame() {
  Impl& self = *impl_;
  if (self.render_ctx == nullptr) {
    return 0;
  }

  // Poll mpv events (state changes, property updates).
  self.PollEvents();

  // Check if a new frame is available.
  const std::uint64_t flags = mpv_render_context_update(self.render_ctx);
  if ((flags & MPV_RENDER_UPDATE_FRAME) == 0U) {
    // Woken (or polled) but mpv has no new frame. Clear the pending flag so
    // the caller can go back to sleeping; keep publishing the last frame.
    self.new_frame_.store(false, std::memory_order_relaxed);
    return self.frame_image_id;
  }

  // Query video dimensions. mpv reports these as int64; read into int64_t
  // locals (MPV_FORMAT_INT64 writes 8 bytes) then narrow to int.
  std::int64_t width64 = 0;
  std::int64_t height64 = 0;
  mpv_get_property(self.mpv, "width", MPV_FORMAT_INT64, &width64);
  mpv_get_property(self.mpv, "height", MPV_FORMAT_INT64, &height64);
  int width = static_cast<int>(width64);
  int height = static_cast<int>(height64);
  if (width <= 0 || height <= 0) {
    width = kFallbackFrameWidth;
    height = kFallbackFrameHeight;
  }
  self.video_width = width;
  self.video_height = height;

  // (Re)allocate the CPU frame buffers only when the video size changes.
  if (self.allocated_w != width || self.allocated_h != height) {
    if (!self.AllocateFrameBuffers(width, height)) {
      LOG(ERROR) << "MpvMediaPlayer: could not allocate a " << width << "x"
                 << height << " CPU frame buffer";
      self.new_frame_.store(false, std::memory_order_relaxed);
      return self.frame_image_id;
    }
    self.allocated_w = width;
    self.allocated_h = height;
  }

  // Render the current mpv frame straight into the CPU buffer. The four
  // mandatory SW parameters are the surface size, its pixel format, its row
  // pitch and its base pointer; mpv scales the video into that surface (black
  // bars included) and touches no GL/GPU state. Both fourccs are mutable locals
  // because mpv's C API takes render_param.data as a non-const void*.
  int surface_size[2] = {width, height};
  char rgba_format[] = "rgba";
  char rgb0_format[] = "rgb0";
  const bool want_alpha = self.sw_format_rgba;
  char* const surface_format = want_alpha ? rgba_format : rgb0_format;
  std::size_t surface_stride = self.stride;
  mpv_render_param render_params[] = {
      {.type = MPV_RENDER_PARAM_SW_SIZE, .data = surface_size},
      {.type = MPV_RENDER_PARAM_SW_FORMAT, .data = surface_format},
      {.type = MPV_RENDER_PARAM_SW_STRIDE, .data = &surface_stride},
      {.type = MPV_RENDER_PARAM_SW_POINTER, .data = self.pixels},
      {.type = MPV_RENDER_PARAM_INVALID, .data = nullptr},
  };

  const int render_ret =
      mpv_render_context_render(self.render_ctx, render_params);

  // The pending signal has been consumed whether or not the render succeeded.
  self.new_frame_.store(false, std::memory_order_relaxed);

  if (render_ret < 0) {
    if (want_alpha) {
      // mpv rejected "rgba". Fall back to the format render.h documents as
      // guaranteed and let the next frame retry; only one downgrade can happen.
      self.sw_format_rgba = false;
      LOG(WARNING) << "MpvMediaPlayer: mpv rejected the software render format "
                      "\"rgba\"; falling back to \"rgb0\" (frames become opaque)";
      return self.frame_image_id;
    }
    LOG(ERROR) << "MpvMediaPlayer: mpv_render_context_render failed: "
               << mpv_error_string(render_ret);
    return self.frame_image_id;
  }

  // Copy the mpv output into the tgfx bitmap, which converts it to the
  // premultiplied layout tgfx rasterizes. The alpha type describes what mpv
  // actually wrote: "rgba" carries unpremultiplied alpha, while "rgb0" leaves
  // the fourth byte undefined, so the frame is declared opaque there. The
  // bitmap -- not |pixels| -- backs the published image, so tgfx owns stable
  // pixel data even though mpv overwrites the frame buffer on the next frame.
  const auto source_info = tgfx::ImageInfo::Make(
      width, height, tgfx::ColorType::RGBA_8888,
      want_alpha ? tgfx::AlphaType::Unpremultiplied : tgfx::AlphaType::Opaque,
      self.stride);
  if (!self.frame_bitmap.writePixels(source_info, self.pixels, 0, 0)) {
    LOG(ERROR) << "MpvMediaPlayer: could not copy the decoded frame into the "
                  "tgfx bitmap";
    return self.frame_image_id;
  }

  auto image = tgfx::Image::MakeFrom(self.frame_bitmap);
  if (image == nullptr) {
    LOG(ERROR) << "MpvMediaPlayer: tgfx::Image::MakeFrom(bitmap) failed";
    return self.frame_image_id;
  }

  // Publish the frame. The first frame allocates the id; every later frame
  // re-binds that same id, so consumers never observe an id going away
  // mid-playback (only TeardownRender() releases it).
  if (self.frame_image_id == 0) {
    self.frame_image_id = FrameImageRegistry::Register(std::move(image));
    if (self.frame_image_id == 0) {
      LOG(ERROR) << "MpvMediaPlayer: could not register the decoded frame";
      return 0;
    }
  } else if (!FrameImageRegistry::Update(self.frame_image_id, std::move(image))) {
    LOG(ERROR) << "MpvMediaPlayer: could not republish the decoded frame under id "
               << self.frame_image_id;
    return self.frame_image_id;
  }

  // Invoke the frame callback via a lock-protected local copy.
  FrameCallback frame_cb;
  {
    std::scoped_lock lock(self.mutex);
    frame_cb = self.frame_callback;
  }
  if (frame_cb != nullptr) {
    frame_cb(self.frame_image_id, width, height);
  }

  return self.frame_image_id;
}

}  // namespace neoflux

#else  // !NEOFLUX_HAS_MPV

#include <glog/logging.h>

#include <atomic>
#include <functional>

namespace neoflux {

// Minimal stub Impl so the pimpl skeleton still compiles without libmpv.
struct MpvMediaPlayer::Impl {
  std::atomic<MediaState> state{MediaState::kError};
};

MpvMediaPlayer::MpvMediaPlayer() : impl_(std::make_unique<Impl>()) {
  LOG(WARNING) << "MpvMediaPlayer: compiled without libmpv support";
}

MpvMediaPlayer::~MpvMediaPlayer() = default;

void MpvMediaPlayer::SetSource(std::string_view source) { (void)source; }
std::string_view MpvMediaPlayer::GetSource() const noexcept { return {}; }
void MpvMediaPlayer::Play() {}
void MpvMediaPlayer::Pause() {}
void MpvMediaPlayer::Stop() {}
void MpvMediaPlayer::Seek(double position_seconds) { (void)position_seconds; }
void MpvMediaPlayer::SetVolume(double volume) { (void)volume; }
double MpvMediaPlayer::GetVolume() const noexcept { return 0.0; }
double MpvMediaPlayer::GetPosition() const noexcept { return 0.0; }
double MpvMediaPlayer::GetDuration() const noexcept { return 0.0; }
MediaState MpvMediaPlayer::GetState() const noexcept { return impl_->state.load(); }
int MpvMediaPlayer::GetVideoWidth() const noexcept { return 0; }
int MpvMediaPlayer::GetVideoHeight() const noexcept { return 0; }
std::uint32_t MpvMediaPlayer::GetRenderUpdateCount() const noexcept { return 0; }
void MpvMediaPlayer::SetStateCallback(StateCallback callback) { (void)callback; }
void MpvMediaPlayer::SetFrameCallback(FrameCallback callback) { (void)callback; }
void MpvMediaPlayer::SetWakeCallback(const std::function<void()>& callback) {
  (void)callback;
}
void MpvMediaPlayer::InitRender() {}
std::uint32_t MpvMediaPlayer::UpdateFrame() { return 0; }
void MpvMediaPlayer::TeardownRender() {}

}  // namespace neoflux

#endif  // NEOFLUX_HAS_MPV
