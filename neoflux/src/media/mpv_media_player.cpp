// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - mpv_media_player.cpp
//
// libmpv media player implementation. Uses the mpv render API to decode video
// frames into an OpenGL texture for compositing.
//
// Pimpl: MpvMediaPlayer::Impl (defined here) owns all mpv/GL state, so neither
// mpv nor OpenGL headers leak into the public mpv_media_player.h.
//
// Threading: see the "THREADING MODEL" block in mpv_media_player.h. In short:
//   - App/UI thread  : Set/GetSource, Play, Pause, Stop, Seek, SetVolume,
//                      SetStateCallback, SetFrameCallback, SetWakeCallback.
//   - Render thread  : InitRender, UpdateTexture (GL context current).
//   - mpv internal   : OnRenderUpdate (frame-available signal, no GL).
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
#include <mpv/render_gl.h>

#include <glog/logging.h>

#include <algorithm>
#include <cstdint>
#include <cstring>

#ifdef NEOFLUX_PLATFORM_DESKTOP
#include <GLFW/glfw3.h>
#endif

#ifdef NEOFLUX_PLATFORM_WINDOWS
#include <windows.h>
#endif

// APIENTRY is defined by <windows.h> (WINAPI = __stdcall on x86). On Linux/macOS
// it is undefined, but GL function pointers are cdecl there, so an empty macro
// is correct. Define it here so the MpvGlLoader typedefs compile off-Windows.
#ifndef APIENTRY
#define APIENTRY
#endif

namespace neoflux {

namespace {

// Minimal GL function loader for FBO/texture operations (same pattern as
// tgfx_renderer.cpp). Loaded via glfwGetProcAddress on the render thread.
//
// The GL ABI fixes these widths for every platform: GLenum, GLuint and
// GLbitfield are 32-bit unsigned, GLint and GLsizei are 32-bit signed. Spelling
// them with <cstdint> types keeps the hand-rolled signatures from drifting with
// the host's `unsigned int` / `int`, which is the whole reason these aliases
// exist instead of including GL.h.
struct MpvGlLoader {
  using GlEnum = std::uint32_t;
  using GlUint = std::uint32_t;
  using GlInt = std::int32_t;
  using GlSizei = std::int32_t;
  using GlBitfield = std::uint32_t;

  void(APIENTRY* GenFramebuffers)(GlSizei, GlUint*) = nullptr;
  void(APIENTRY* DeleteFramebuffers)(GlSizei, const GlUint*) = nullptr;
  void(APIENTRY* BindFramebuffer)(GlEnum, GlUint) = nullptr;
  void(APIENTRY* FramebufferTexture2D)(GlEnum, GlEnum, GlEnum, GlUint, GlInt) = nullptr;
  void(APIENTRY* GenTextures)(GlSizei, GlUint*) = nullptr;
  void(APIENTRY* DeleteTextures)(GlSizei, const GlUint*) = nullptr;
  void(APIENTRY* BindTexture)(GlEnum, GlUint) = nullptr;
  void(APIENTRY* TexImage2D)(GlEnum, GlInt, GlInt, GlSizei, GlSizei, GlInt, GlEnum,
                             GlEnum, const void*) = nullptr;
  void(APIENTRY* TexParameteri)(GlEnum, GlEnum, GlInt) = nullptr;

  void Load() {
    GenFramebuffers = reinterpret_cast<decltype(GenFramebuffers)>(
        glfwGetProcAddress("glGenFramebuffers"));
    DeleteFramebuffers = reinterpret_cast<decltype(DeleteFramebuffers)>(
        glfwGetProcAddress("glDeleteFramebuffers"));
    BindFramebuffer = reinterpret_cast<decltype(BindFramebuffer)>(
        glfwGetProcAddress("glBindFramebuffer"));
    FramebufferTexture2D = reinterpret_cast<decltype(FramebufferTexture2D)>(
        glfwGetProcAddress("glFramebufferTexture2D"));
    GenTextures = reinterpret_cast<decltype(GenTextures)>(
        glfwGetProcAddress("glGenTextures"));
    DeleteTextures = reinterpret_cast<decltype(DeleteTextures)>(
        glfwGetProcAddress("glDeleteTextures"));
    BindTexture = reinterpret_cast<decltype(BindTexture)>(
        glfwGetProcAddress("glBindTexture"));
    TexImage2D = reinterpret_cast<decltype(TexImage2D)>(
        glfwGetProcAddress("glTexImage2D"));
    TexParameteri = reinterpret_cast<decltype(TexParameteri)>(
        glfwGetProcAddress("glTexParameteri"));
  }
};

MpvGlLoader& GetGlLoader() {
  static MpvGlLoader loader;
  return loader;
}

// GL constants (avoid including GL.h which conflicts with our custom loader).
// Typed to match the parameter each one is passed to: GLenum (u32) vs GLint
// (i32). Every value below fits comfortably in the signed range that matters.
constexpr std::uint32_t kGlTexture2d = 0x0DE1;
constexpr std::uint32_t kGlTextureMinFilter = 0x2801;
constexpr std::uint32_t kGlTextureMagFilter = 0x2800;
constexpr std::uint32_t kGlTextureWrapS = 0x2802;
constexpr std::uint32_t kGlTextureWrapT = 0x2803;
constexpr std::int32_t kGlLinear = 0x2601;
constexpr std::int32_t kGlClampToEdge = 0x812F;
constexpr std::uint32_t kGlFramebuffer = 0x8D40;
constexpr std::int32_t kGlRgba = 0x1908;
constexpr std::uint32_t kGlUnsignedByte = 0x1401;
constexpr std::uint32_t kGlColorAttachment0 = 0x8CE0;

// OpenGL get_proc_address callback for mpv render context.
void* GetProcAddress(void* /*ctx*/, const char* name) {
  return reinterpret_cast<void*>(glfwGetProcAddress(name));
}

}  // namespace

// =============================================================================
// MpvMediaPlayer::Impl - all mpv/GL state and private operations live here.
// =============================================================================
struct MpvMediaPlayer::Impl {
  Impl() = default;
  ~Impl() {
    // Detach all user callbacks first so none fires while we tear down mpv/GL.
    {
      std::scoped_lock lock(mutex);
      wake_callback = nullptr;
      state_callback = nullptr;
      frame_callback = nullptr;
    }

    // GL teardown. In the framework, MediaWidget already ran TeardownRender() on
    // the render thread (which owns the GL context) BEFORE this destructor runs
    // on the App thread, so |render_ctx| is nullptr here and there is nothing GL
    // to free. If TeardownRender() was never called (unit tests that own a GL
    // context on the calling thread, or a player whose render context was never
    // created), free inline -- valid ONLY because the calling thread itself owns
    // the current GL context. Never free |render_ctx|/texture/fbo on the App
    // thread in production: that thread has no current OpenGL context.
    if (render_ctx != nullptr) {
      FreeRenderContextAndGlResources();
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

  // Frees the mpv render context and the cached GL texture/FBO. MUST be called
  // on the render thread with the OpenGL context current (this is where those
  // objects were created). Idempotent: safe to call when nothing was allocated.
  // Invoked either by TeardownRender() (framework, render thread) or inline from
  // ~Impl() (tests, where the calling thread owns the GL context).
  void FreeRenderContextAndGlResources() {
    if (render_ctx != nullptr) {
      mpv_render_context_set_update_callback(render_ctx, nullptr, nullptr);
      mpv_render_context_free(render_ctx);
      render_ctx = nullptr;
    }
    auto& gl = GetGlLoader();
    if (fbo_id != 0 && gl.DeleteFramebuffers != nullptr) {
      gl.DeleteFramebuffers(1, &fbo_id);
      fbo_id = 0;
    }
    if (texture_id != 0 && gl.DeleteTextures != nullptr) {
      gl.DeleteTextures(1, &texture_id);
      texture_id = 0;
    }
    allocated_w = 0;
    allocated_h = 0;
    render_initialized = false;
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
  // whichever thread calls UpdateTexture() (the render thread in production).
  void PollEvents();

  // Sets an mpv property as double.
  void SetPropertyDouble(const char* name, double value);

  // Gets an mpv property as double. Returns 0 on failure.
  [[nodiscard]] double GetPropertyDouble(const char* name) const;

  mpv_handle* mpv = nullptr;
  mpv_render_context* render_ctx = nullptr;
  // Cached GL objects. The FBO is created once and reused every frame; the
  // texture storage (TexImage2D) is (re)allocated only when the size changes.
  std::uint32_t texture_id = 0;
  std::uint32_t fbo_id = 0;
  // Dimensions of the currently-allocated texture storage (0 = unallocated).
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
  // |frame_cv_|. The render thread consumes the flag in UpdateTexture().
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
// GL and MUST NOT block. The actual frame pickup (GL) happens in UpdateTexture
// on the render thread.
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
  // non-blocking and must not touch GL.
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
  // Render thread, GL context current. Free the mpv render context and the
  // cached GL texture/FBO on the thread that owns the context. After this, the
  // App thread may destroy the player (mpv_terminate_destroy touches no GL).
  // FreeRenderContextAndGlResources() is a no-op if nothing was ever allocated.
  impl_->FreeRenderContextAndGlResources();
}

void MpvMediaPlayer::InitRender() {
  if (impl_->mpv == nullptr || impl_->render_initialized) {
    return;
  }

  // Load GL entry points required for FBO/texture management.
  GetGlLoader().Load();

  mpv_opengl_init_params gl_init_params{};
  gl_init_params.get_proc_address = GetProcAddress;
  gl_init_params.get_proc_address_ctx = nullptr;

  // mpv's C API types render_param.data as a non-const void*, so make a mutable
  // copy of the API-type string literal instead of casting away const.
  char api_type[] = MPV_RENDER_API_TYPE_OPENGL;
  mpv_render_param params[] = {
      {.type = MPV_RENDER_PARAM_API_TYPE, .data = api_type},
      {.type = MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, .data = &gl_init_params},
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
  LOG(INFO) << "MpvMediaPlayer: render context initialized";
}

std::uint32_t MpvMediaPlayer::UpdateTexture() {
  Impl& self = *impl_;
  if (self.render_ctx == nullptr) {
    return 0;
  }

  auto& gl = GetGlLoader();
  if (gl.GenFramebuffers == nullptr) {
    return 0;
  }

  // Poll mpv events (state changes, property updates).
  self.PollEvents();

  // Check if a new frame is available.
  const std::uint64_t flags = mpv_render_context_update(self.render_ctx);
  if ((flags & MPV_RENDER_UPDATE_FRAME) == 0U) {
    // Woken (or polled) but mpv has no new frame. Clear the pending flag so
    // the caller can go back to sleeping; reuse the existing texture.
    self.new_frame_.store(false, std::memory_order_relaxed);
    return self.texture_id;
  }

  // Query video dimensions. mpv reports these as int64; read into int64_t
  // locals (MPV_FORMAT_INT64 writes 8 bytes) then narrow to int for GL.
  std::int64_t width64 = 0;
  std::int64_t height64 = 0;
  mpv_get_property(self.mpv, "width", MPV_FORMAT_INT64, &width64);
  mpv_get_property(self.mpv, "height", MPV_FORMAT_INT64, &height64);
  int width = static_cast<int>(width64);
  int height = static_cast<int>(height64);
  if (width <= 0 || height <= 0) {
    width = 640;
    height = 360;
  }
  self.video_width = width;
  self.video_height = height;

  // Create the texture once on first use.
  if (self.texture_id == 0) {
    gl.GenTextures(1, &self.texture_id);
    gl.BindTexture(kGlTexture2d, self.texture_id);
    gl.TexParameteri(kGlTexture2d, kGlTextureMinFilter, kGlLinear);
    gl.TexParameteri(kGlTexture2d, kGlTextureMagFilter, kGlLinear);
    gl.TexParameteri(kGlTexture2d, kGlTextureWrapS, kGlClampToEdge);
    gl.TexParameteri(kGlTexture2d, kGlTextureWrapT, kGlClampToEdge);
  }

  // Create the FBO once; it is cached and reused for every frame.
  if (self.fbo_id == 0) {
    gl.GenFramebuffers(1, &self.fbo_id);
  }

  // (Re)allocate texture storage ONLY when the size changes. Reusing the
  // existing storage on steady-size frames avoids a per-frame TexImage2D
  // allocation churn.
  if (self.allocated_w != width || self.allocated_h != height) {
    gl.BindTexture(kGlTexture2d, self.texture_id);
    gl.TexImage2D(kGlTexture2d, 0, kGlRgba, width, height, 0, kGlRgba,
                  kGlUnsignedByte, nullptr);
    self.allocated_w = width;
    self.allocated_h = height;
  }

  // Bind the cached FBO and attach the texture, then render the current mpv
  // frame into it.
  gl.BindFramebuffer(kGlFramebuffer, self.fbo_id);
  gl.FramebufferTexture2D(kGlFramebuffer, kGlColorAttachment0, kGlTexture2d,
                          self.texture_id, 0);

  mpv_opengl_fbo fbo_params{};
  fbo_params.fbo = static_cast<int>(self.fbo_id);
  fbo_params.w = width;
  fbo_params.h = height;
  fbo_params.internal_format = 0;

  mpv_render_param render_params[] = {
      {.type = MPV_RENDER_PARAM_OPENGL_FBO, .data = &fbo_params},
      {.type = MPV_RENDER_PARAM_INVALID, .data = nullptr},
  };

  const int render_ret =
      mpv_render_context_render(self.render_ctx, render_params);
  if (render_ret < 0) {
    LOG(ERROR) << "MpvMediaPlayer: mpv_render_context_render failed: "
               << mpv_error_string(render_ret);
  }

  gl.BindFramebuffer(kGlFramebuffer, 0);

  // The new frame has been composited; consume the pending signal.
  self.new_frame_.store(false, std::memory_order_relaxed);

  // Invoke the frame callback via a lock-protected local copy.
  FrameCallback frame_cb;
  {
    std::scoped_lock lock(self.mutex);
    frame_cb = self.frame_callback;
  }
  if (frame_cb != nullptr) {
    frame_cb(self.texture_id, width, height);
  }

  return self.texture_id;
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
std::uint32_t MpvMediaPlayer::UpdateTexture() { return 0; }
void MpvMediaPlayer::TeardownRender() {}

}  // namespace neoflux

#endif  // NEOFLUX_HAS_MPV
