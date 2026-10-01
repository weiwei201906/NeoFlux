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
// =============================================================================

#include "neoflux/media/mpv_media_player.h"

#include <atomic>
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

namespace neoflux {

namespace {

// Minimal GL function loader for FBO/texture operations (same pattern as
// tgfx_renderer.cpp). Loaded via glfwGetProcAddress on the render thread.
struct MpvGlLoader {
  using GlEnum = unsigned int;
  using GlUint = unsigned int;
  using GlInt = int;
  using GlSizei = int;
  using GlBitfield = unsigned int;

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
constexpr unsigned int kGlTexture2d = 0x0DE1;
constexpr unsigned int kGlTextureMinFilter = 0x2801;
constexpr unsigned int kGlTextureMagFilter = 0x2800;
constexpr unsigned int kGlTextureWrapS = 0x2802;
constexpr unsigned int kGlTextureWrapT = 0x2803;
constexpr int kGlLinear = 0x2601;
constexpr int kGlClampToEdge = 0x812F;
constexpr unsigned int kGlFramebuffer = 0x8D40;
constexpr int kGlRgba = 0x1908;
constexpr unsigned int kGlUnsignedByte = 0x1401;
constexpr unsigned int kGlColorAttachment0 = 0x8CE0;

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
    if (render_ctx != nullptr) {
      mpv_render_context_free(render_ctx);
      render_ctx = nullptr;
    }
    if (mpv != nullptr) {
      mpv_terminate_destroy(mpv);
      mpv = nullptr;
    }
    if (texture_id != 0) {
      // Texture deletion must happen on the render thread with a current GL
      // context. We leak it here intentionally; the render layer owns GL
      // resource lifetime. In practice this runs during app shutdown when the
      // GL context is still current.
      auto& gl = GetGlLoader();
      if (gl.DeleteTextures != nullptr) {
        gl.DeleteTextures(1, &texture_id);
      }
      texture_id = 0;
    }
  }

  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(Impl&&) = delete;

  // Creates the mpv handle and configures basic options. Returns true on
  // success.
  bool CreateMpvHandle();

  // mpv event handler. Polls events and updates state/properties.
  void PollEvents();

  // Sets an mpv property as double.
  void SetPropertyDouble(const char* name, double value);

  // Gets an mpv property as double. Returns 0 on failure.
  [[nodiscard]] double GetPropertyDouble(const char* name) const;

  // Sends a command to mpv (e.g. "loadfile", "pause").
  void Command(const char* args[]);

  mpv_handle* mpv = nullptr;
  mpv_render_context* render_ctx = nullptr;
  std::uint32_t texture_id = 0;
  int video_width = 0;
  int video_height = 0;
  double volume = 1.0;
  MediaState state = MediaState::kIdle;
  std::string source;
  StateCallback state_callback;
  FrameCallback frame_callback;
  std::mutex mutex;
  bool render_initialized = false;
  // Incremented by OnMpvRenderUpdate on an arbitrary mpv-internal thread each
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
  mpv_set_option_string(mpv, "msg-level", "all=no");
  mpv_set_option_string(mpv, "ytdl", "no");

  const int ret = mpv_initialize(mpv);
  if (ret < 0) {
    LOG(ERROR) << "MpvMediaPlayer: mpv_initialize failed: " << mpv_error_string(ret);
    mpv_terminate_destroy(mpv);
    mpv = nullptr;
    return false;
  }

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
        state = MediaState::kPlaying;
        SetPropertyDouble("pause", 0.0);
        if (state_callback != nullptr) {
          state_callback(state);
        }
        break;
      case MPV_EVENT_END_FILE:
        state = MediaState::kEnded;
        if (state_callback != nullptr) {
          state_callback(state);
        }
        break;
      case MPV_EVENT_IDLE:
        state = MediaState::kIdle;
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
    impl_->state = MediaState::kError;
  }
}

MpvMediaPlayer::~MpvMediaPlayer() = default;

void MpvMediaPlayer::SetSource(std::string_view source) {
  std::scoped_lock lock(impl_->mutex);
  impl_->source = std::string(source);
}

std::string_view MpvMediaPlayer::GetSource() const noexcept {
  return impl_->source;
}

void MpvMediaPlayer::Play() {
  if (impl_->mpv == nullptr || impl_->source.empty()) {
    return;
  }
  std::scoped_lock lock(impl_->mutex);
  const char* cmd[] = {"loadfile", impl_->source.c_str(), nullptr};
  mpv_command_async(impl_->mpv, 0, cmd);
  impl_->state = MediaState::kLoading;
  if (impl_->state_callback != nullptr) {
    impl_->state_callback(impl_->state);
  }
}

void MpvMediaPlayer::Pause() {
  if (impl_->mpv == nullptr) {
    return;
  }
  impl_->SetPropertyDouble("pause", 1.0);
  impl_->state = MediaState::kPaused;
  if (impl_->state_callback != nullptr) {
    impl_->state_callback(impl_->state);
  }
}

void MpvMediaPlayer::Stop() {
  if (impl_->mpv == nullptr) {
    return;
  }
  const char* cmd[] = {"stop", nullptr};
  mpv_command_async(impl_->mpv, 0, cmd);
  impl_->state = MediaState::kIdle;
  if (impl_->state_callback != nullptr) {
    impl_->state_callback(impl_->state);
  }
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
  return impl_->state;
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
// MpvMediaPlayer* (this). It runs on an arbitrary mpv-internal thread, so it
// must do as little as possible: only bump the atomic observability counter.
// The actual frame pickup happens in UpdateTexture() on the render thread.
// Being a static member lets it reach the private impl_ without a friend, and
// exposes no mpv/GL types in the header.
void MpvMediaPlayer::OnRenderUpdate(void* ctx) {
  auto* self = static_cast<MpvMediaPlayer*>(ctx);
  if (self != nullptr) {
    self->impl_->update_count_.fetch_add(1, std::memory_order_relaxed);
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
    return self.texture_id;
  }

  // Create the texture on first use.
  if (self.texture_id == 0) {
    gl.GenTextures(1, &self.texture_id);
    gl.BindTexture(kGlTexture2d, self.texture_id);
    gl.TexParameteri(kGlTexture2d, kGlTextureMinFilter, kGlLinear);
    gl.TexParameteri(kGlTexture2d, kGlTextureMagFilter, kGlLinear);
    gl.TexParameteri(kGlTexture2d, kGlTextureWrapS, kGlClampToEdge);
    gl.TexParameteri(kGlTexture2d, kGlTextureWrapT, kGlClampToEdge);
  }

  // Render the current mpv frame into an FBO backed by our texture.
  std::uint32_t fbo = 0;
  gl.GenFramebuffers(1, &fbo);
  gl.BindFramebuffer(kGlFramebuffer, fbo);

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

  // Allocate texture storage.
  gl.BindTexture(kGlTexture2d, self.texture_id);
  gl.TexImage2D(kGlTexture2d, 0, kGlRgba, width, height, 0, kGlRgba,
                kGlUnsignedByte, nullptr);
  gl.FramebufferTexture2D(kGlFramebuffer, kGlColorAttachment0, kGlTexture2d,
                          self.texture_id, 0);

  mpv_opengl_fbo fbo_params{};
  fbo_params.fbo = static_cast<int>(fbo);
  fbo_params.w = width;
  fbo_params.h = height;
  fbo_params.internal_format = 0;

  mpv_render_param render_params[] = {
      {.type = MPV_RENDER_PARAM_OPENGL_FBO, .data = &fbo_params},
      {.type = MPV_RENDER_PARAM_INVALID, .data = nullptr},
  };

  mpv_render_context_render(self.render_ctx, render_params);

  gl.BindFramebuffer(kGlFramebuffer, 0);
  gl.DeleteFramebuffers(1, &fbo);

  if (self.frame_callback != nullptr) {
    self.frame_callback(self.texture_id, width, height);
  }

  return self.texture_id;
}

}  // namespace neoflux

#else  // !NEOFLUX_HAS_MPV

#include <glog/logging.h>

namespace neoflux {

// Minimal stub Impl so the pimpl skeleton still compiles without libmpv.
struct MpvMediaPlayer::Impl {
  MediaState state = MediaState::kError;
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
MediaState MpvMediaPlayer::GetState() const noexcept { return impl_->state; }
int MpvMediaPlayer::GetVideoWidth() const noexcept { return 0; }
int MpvMediaPlayer::GetVideoHeight() const noexcept { return 0; }
std::uint32_t MpvMediaPlayer::GetRenderUpdateCount() const noexcept { return 0; }
void MpvMediaPlayer::SetStateCallback(StateCallback callback) { (void)callback; }
void MpvMediaPlayer::SetFrameCallback(FrameCallback callback) { (void)callback; }
void MpvMediaPlayer::InitRender() {}
std::uint32_t MpvMediaPlayer::UpdateTexture() { return 0; }

}  // namespace neoflux

#endif  // NEOFLUX_HAS_MPV
