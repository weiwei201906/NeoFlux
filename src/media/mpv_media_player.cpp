// =============================================================================
// NeoFlux - mpv_media_player.cpp
//
// libmpv media player implementation. Uses the mpv render API to decode video
// frames into an OpenGL texture for compositing.
// =============================================================================

#include "neoflux/media/mpv_media_player.h"

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

// mpv render update callback: invoked when a new frame is available.
void OnMpvRenderUpdate(void* ctx) {
  auto* self = static_cast<MpvMediaPlayer*>(ctx);
  (void)self;  // Actual frame pickup happens in UpdateTexture on render thread.
}

}  // namespace

MpvMediaPlayer::MpvMediaPlayer() {
  if (!CreateMpvHandle()) {
    state_ = MediaState::kError;
  }
}

MpvMediaPlayer::~MpvMediaPlayer() {
  if (render_ctx_ != nullptr) {
    mpv_render_context_free(render_ctx_);
    render_ctx_ = nullptr;
  }
  if (mpv_ != nullptr) {
    mpv_terminate_destroy(mpv_);
    mpv_ = nullptr;
  }
  if (texture_id_ != 0) {
    // Texture deletion must happen on the render thread with a current GL
    // context. We leak it here intentionally; the render layer owns GL
    // resource lifetime. In practice this is called during app shutdown when
    // the GL context is still current.
    auto& gl = GetGlLoader();
    if (gl.DeleteTextures != nullptr) {
      gl.DeleteTextures(1, &texture_id_);
    }
    texture_id_ = 0;
  }
}

bool MpvMediaPlayer::CreateMpvHandle() {
  mpv_ = mpv_create();
  if (mpv_ == nullptr) {
    LOG(ERROR) << "MpvMediaPlayer: mpv_create failed";
    return false;
  }

  // Configure mpv for embedded rendering: no window, video output via render API.
  mpv_set_option_string(mpv_, "no-video", "no");
  mpv_set_option_string(mpv_, "vo", "libmpv");
  mpv_set_option_string(mpv_, "terminal", "no");
  mpv_set_option_string(mpv_, "msg-level", "all=no");
  mpv_set_option_string(mpv_, "ytdl", "no");

  const int ret = mpv_initialize(mpv_);
  if (ret < 0) {
    LOG(ERROR) << "MpvMediaPlayer: mpv_initialize failed: " << mpv_error_string(ret);
    mpv_terminate_destroy(mpv_);
    mpv_ = nullptr;
    return false;
  }

  return true;
}

void MpvMediaPlayer::SetSource(std::string_view source) {
  std::lock_guard<std::mutex> lock(mutex_);
  source_ = std::string(source);
}

std::string_view MpvMediaPlayer::GetSource() const noexcept {
  return source_;
}

void MpvMediaPlayer::Play() {
  if (mpv_ == nullptr || source_.empty()) {
    return;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  const char* cmd[] = {"loadfile", source_.c_str(), nullptr};
  mpv_command_async(mpv_, 0, cmd);
  state_ = MediaState::kLoading;
  if (state_callback_ != nullptr) {
    state_callback_(state_);
  }
}

void MpvMediaPlayer::Pause() {
  if (mpv_ == nullptr) {
    return;
  }
  SetPropertyDouble("pause", 1.0);
  state_ = MediaState::kPaused;
  if (state_callback_ != nullptr) {
    state_callback_(state_);
  }
}

void MpvMediaPlayer::Stop() {
  if (mpv_ == nullptr) {
    return;
  }
  const char* cmd[] = {"stop", nullptr};
  mpv_command_async(mpv_, 0, cmd);
  state_ = MediaState::kIdle;
  if (state_callback_ != nullptr) {
    state_callback_(state_);
  }
}

void MpvMediaPlayer::Seek(double position_seconds) {
  if (mpv_ == nullptr) {
    return;
  }
  char target[32];
  std::snprintf(target, sizeof(target), "%f", position_seconds);
  const char* cmd[] = {"seek", target, "absolute", nullptr};
  mpv_command_async(mpv_, 0, cmd);
}

void MpvMediaPlayer::SetVolume(double volume) {
  volume_ = std::clamp(volume, 0.0, 1.0);
  SetPropertyDouble("volume", volume_ * 100.0);
}

double MpvMediaPlayer::GetVolume() const noexcept {
  return volume_;
}

double MpvMediaPlayer::GetPosition() const noexcept {
  return GetPropertyDouble("time-pos");
}

double MpvMediaPlayer::GetDuration() const noexcept {
  return GetPropertyDouble("duration");
}

MediaState MpvMediaPlayer::GetState() const noexcept {
  return state_;
}

int MpvMediaPlayer::GetVideoWidth() const noexcept {
  return video_width_;
}

int MpvMediaPlayer::GetVideoHeight() const noexcept {
  return video_height_;
}

void MpvMediaPlayer::SetStateCallback(StateCallback callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  state_callback_ = std::move(callback);
}

void MpvMediaPlayer::SetFrameCallback(FrameCallback callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  frame_callback_ = std::move(callback);
}

void MpvMediaPlayer::InitRender() {
  if (mpv_ == nullptr || render_initialized_) {
    return;
  }

  // Load GL entry points required for FBO/texture management.
  GetGlLoader().Load();

  mpv_opengl_init_params gl_init_params{};
  gl_init_params.get_proc_address = GetProcAddress;
  gl_init_params.get_proc_address_ctx = nullptr;

  // NOLINTNEXTLINE(modernize-use-designated-initializers,cppcoreguidelines-pro-type-const-cast)
  mpv_render_param params[] = {
      {MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL)},  // NOLINT
      {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
      {MPV_RENDER_PARAM_INVALID, nullptr},
  };

  const int ret = mpv_render_context_create(&render_ctx_, mpv_, params);
  if (ret < 0) {
    LOG(ERROR) << "MpvMediaPlayer: mpv_render_context_create failed: "
               << mpv_error_string(ret);
    return;
  }

  mpv_render_context_set_update_callback(render_ctx_, OnMpvRenderUpdate, this);
  render_initialized_ = true;
  LOG(INFO) << "MpvMediaPlayer: render context initialized";
}

std::uint32_t MpvMediaPlayer::UpdateTexture() {
  if (render_ctx_ == nullptr) {
    return 0;
  }

  auto& gl = GetGlLoader();
  if (gl.GenFramebuffers == nullptr) {
    return 0;
  }

  // Poll mpv events (state changes, property updates).
  PollEvents();

  // Check if a new frame is available.
  const std::uint64_t flags = mpv_render_context_update(render_ctx_);
  if ((flags & MPV_RENDER_UPDATE_FRAME) == 0U) {
    return texture_id_;
  }

  // Create the texture on first use.
  if (texture_id_ == 0) {
    gl.GenTextures(1, &texture_id_);
    gl.BindTexture(kGlTexture2d, texture_id_);
    gl.TexParameteri(kGlTexture2d, kGlTextureMinFilter, kGlLinear);
    gl.TexParameteri(kGlTexture2d, kGlTextureMagFilter, kGlLinear);
    gl.TexParameteri(kGlTexture2d, kGlTextureWrapS, kGlClampToEdge);
    gl.TexParameteri(kGlTexture2d, kGlTextureWrapT, kGlClampToEdge);
  }

  // Render the current mpv frame into an FBO backed by our texture.
  std::uint32_t fbo = 0;
  gl.GenFramebuffers(1, &fbo);
  gl.BindFramebuffer(kGlFramebuffer, fbo);

  // Query video dimensions.
  int width = 0;
  int height = 0;
  mpv_get_property(mpv_, "width", MPV_FORMAT_INT64, &width);
  mpv_get_property(mpv_, "height", MPV_FORMAT_INT64, &height);
  if (width <= 0 || height <= 0) {
    width = 640;
    height = 360;
  }
  video_width_ = width;
  video_height_ = height;

  // Allocate texture storage.
  gl.BindTexture(kGlTexture2d, texture_id_);
  gl.TexImage2D(kGlTexture2d, 0, kGlRgba, width, height, 0, kGlRgba,
                kGlUnsignedByte, nullptr);
  gl.FramebufferTexture2D(kGlFramebuffer, kGlColorAttachment0, kGlTexture2d,
                          texture_id_, 0);

  mpv_opengl_fbo fbo_params{};
  fbo_params.fbo = static_cast<int>(fbo);
  fbo_params.w = width;
  fbo_params.h = height;
  fbo_params.internal_format = 0;

  // NOLINTNEXTLINE(modernize-use-designated-initializers)
  mpv_render_param render_params[] = {
      {MPV_RENDER_PARAM_OPENGL_FBO, &fbo_params},
      {MPV_RENDER_PARAM_INVALID, nullptr},
  };

  mpv_render_context_render(render_ctx_, render_params);

  gl.BindFramebuffer(kGlFramebuffer, 0);
  gl.DeleteFramebuffers(1, &fbo);

  if (frame_callback_ != nullptr) {
    frame_callback_(texture_id_, width, height);
  }

  return texture_id_;
}

void MpvMediaPlayer::PollEvents() {
  if (mpv_ == nullptr) {
    return;
  }
  while (true) {
    mpv_event* event = mpv_wait_event(mpv_, 0);
    if (event == nullptr || event->event_id == MPV_EVENT_NONE) {
      break;
    }
    switch (event->event_id) {
      case MPV_EVENT_FILE_LOADED:
        state_ = MediaState::kPlaying;
        SetPropertyDouble("pause", 0.0);
        if (state_callback_ != nullptr) state_callback_(state_);
        break;
      case MPV_EVENT_END_FILE:
        state_ = MediaState::kEnded;
        if (state_callback_ != nullptr) state_callback_(state_);
        break;
      case MPV_EVENT_IDLE:
        state_ = MediaState::kIdle;
        break;
      default:
        break;
    }
  }
}

void MpvMediaPlayer::SetPropertyDouble(const char* name, double value) {
  if (mpv_ == nullptr) {
    return;
  }
  mpv_set_property(mpv_, name, MPV_FORMAT_DOUBLE, &value);
}

double MpvMediaPlayer::GetPropertyDouble(const char* name) const {
  if (mpv_ == nullptr) return 0.0;
  double value = 0.0;
  mpv_get_property(mpv_, name, MPV_FORMAT_DOUBLE, &value);
  return value;
}

void MpvMediaPlayer::Command(const char* args[]) {
  if (mpv_ == nullptr) {
    return;
  }
  mpv_command_async(mpv_, 0, args);
}

}  // namespace neoflux

#else  // !NEOFLUX_HAS_MPV

#include <glog/logging.h>

namespace neoflux {

MpvMediaPlayer::MpvMediaPlayer() : state_(MediaState::kError) {
  LOG(WARNING) << "MpvMediaPlayer: compiled without libmpv support";
}

MpvMediaPlayer::~MpvMediaPlayer() = default;

bool MpvMediaPlayer::CreateMpvHandle() { return false; }
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
MediaState MpvMediaPlayer::GetState() const noexcept { return state_; }
int MpvMediaPlayer::GetVideoWidth() const noexcept { return 0; }
int MpvMediaPlayer::GetVideoHeight() const noexcept { return 0; }
void MpvMediaPlayer::SetStateCallback(StateCallback callback) { (void)callback; }
void MpvMediaPlayer::SetFrameCallback(FrameCallback callback) { (void)callback; }
void MpvMediaPlayer::InitRender() {}
std::uint32_t MpvMediaPlayer::UpdateTexture() { return 0; }
void MpvMediaPlayer::PollEvents() {}
void MpvMediaPlayer::SetPropertyDouble(const char* name, double value) {
  (void)name;
  (void)value;
}
double MpvMediaPlayer::GetPropertyDouble(const char* name) const {
  (void)name;
  return 0.0;
}
void MpvMediaPlayer::Command(const char* args[]) { (void)args; }

}  // namespace neoflux

#endif  // NEOFLUX_HAS_MPV
