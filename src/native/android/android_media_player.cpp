// =============================================================================
// NeoFlux - android_media_player.cpp
//
// Android media player implementation using JNI to bridge android.media.MediaPlayer
// and android.graphics.SurfaceTexture. Video frames are rendered to an OpenGL ES
// texture via SurfaceTexture, which the NeoFlux render layer composites.
//
// Compiled only on Android (ANDROID macro). Requires NDK with JNI and GLES.
// =============================================================================

#include "neoflux/native/android/android_media_player.h"

#ifdef ANDROID

#include <android/native_window.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <glog/logging.h>

#include <cstring>

namespace neoflux {

JavaVM* AndroidMediaPlayer::java_vm_ = nullptr;

namespace {

// JNI_OnLoad is called by the JVM when the native library is loaded.
// We cache the JavaVM pointer for later use.
jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
  AndroidMediaPlayer::SetJavaVm(vm);
  return JNI_VERSION_1_6;
}

}  // namespace

// static
void AndroidMediaPlayer::SetJavaVm(JavaVM* vm) {
  java_vm_ = vm;
}

AndroidMediaPlayer::AndroidMediaPlayer() {
  if (!InitJni()) {
    state_ = MediaState::kError;
  }
}

AndroidMediaPlayer::~AndroidMediaPlayer() {
  Release();
}

bool AndroidMediaPlayer::InitJni() {
  if (java_vm_ == nullptr) {
    LOG(ERROR) << "AndroidMediaPlayer: JavaVM not set (JNI_OnLoad not called?)";
    return false;
  }

  JNIEnv* env = GetEnv();
  if (env == nullptr) {
    LOG(ERROR) << "AndroidMediaPlayer: failed to get JNIEnv";
    return false;
  }

  // Find and cache class references (must be global refs).
  jclass local_mp = env->FindClass("android/media/MediaPlayer");
  if (local_mp == nullptr) {
    LOG(ERROR) << "AndroidMediaPlayer: MediaPlayer class not found";
    return false;
  }
  media_player_class_ = static_cast<jclass>(env->NewGlobalRef(local_mp));
  env->DeleteLocalRef(local_mp);

  jclass local_st = env->FindClass("android/graphics/SurfaceTexture");
  if (local_st == nullptr) {
    LOG(ERROR) << "AndroidMediaPlayer: SurfaceTexture class not found";
    return false;
  }
  surface_texture_class_ = static_cast<jclass>(env->NewGlobalRef(local_st));
  env->DeleteLocalRef(local_st);

  jni_initialized_ = true;
  LOG(INFO) << "AndroidMediaPlayer: JNI initialized";
  return true;
}

JNIEnv* AndroidMediaPlayer::GetEnv() const {
  if (java_vm_ == nullptr) {
    return nullptr;
  }
  JNIEnv* env = nullptr;
  const jint result = java_vm_->GetEnv(
      reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
  if (result == JNI_EDETACHED) {
    if (java_vm_->AttachCurrentThread(&env, nullptr) != JNI_OK) {
      return nullptr;
    }
  }
  return env;
}

void AndroidMediaPlayer::DetachThread() const {
  if (java_vm_ != nullptr) {
    java_vm_->DetachCurrentThread();
  }
}

bool AndroidMediaPlayer::CreateSurfaceTexture() {
  JNIEnv* env = GetEnv();
  if (env == nullptr) {
    return false;
  }

  // Generate the GL texture that SurfaceTexture will render into.
  glGenTextures(1, &texture_id_);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_id_);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Create SurfaceTexture(texture_id).
  jmethodID ctor = env->GetMethodID(
      surface_texture_class_, "<init>", "(I)V");
  if (ctor == nullptr) {
    LOG(ERROR) << "AndroidMediaPlayer: SurfaceTexture constructor not found";
    return false;
  }
  jobject local_st = env->NewObject(surface_texture_class_, ctor, texture_id_);
  if (local_st == nullptr) {
    LOG(ERROR) << "AndroidMediaPlayer: failed to create SurfaceTexture";
    return false;
  }
  surface_texture_ = env->NewGlobalRef(local_st);
  env->DeleteLocalRef(local_st);

  // Create Surface(surfaceTexture) for MediaPlayer.
  jclass surface_class = env->FindClass("android/view/Surface");
  if (surface_class == nullptr) {
    LOG(ERROR) << "AndroidMediaPlayer: Surface class not found";
    return false;
  }
  jmethodID surface_ctor = env->GetMethodID(
      surface_class, "<init>",
      "(Landroid/graphics/SurfaceTexture;)V");
  jobject local_surface = env->NewObject(
      surface_class, surface_ctor, surface_texture_);
  if (local_surface == nullptr) {
    LOG(ERROR) << "AndroidMediaPlayer: failed to create Surface";
    return false;
  }
  surface_ = env->NewGlobalRef(local_surface);
  env->DeleteLocalRef(local_surface);
  env->DeleteLocalRef(surface_class);

  render_initialized_ = true;
  LOG(INFO) << "AndroidMediaPlayer: SurfaceTexture created (tex=" << texture_id_
            << ")";
  return true;
}

void AndroidMediaPlayer::InitRender() {
  if (render_initialized_) {
    return;
  }
  CreateSurfaceTexture();
}

std::uint32_t AndroidMediaPlayer::UpdateTexture() {
  if (surface_texture_ == nullptr) {
    return 0;
  }

  JNIEnv* env = GetEnv();
  if (env == nullptr) {
    return texture_id_;
  }

  // Update the SurfaceTexture with the latest video frame.
  jmethodID update_method = env->GetMethodID(
      surface_texture_class_, "updateTexImage", "()V");
  if (update_method != nullptr) {
    env->CallVoidMethod(surface_texture_, update_method);
  }

  // Query transformation matrix (not strictly needed for basic rendering).
  if (frame_callback_ != nullptr) {
    frame_callback_(texture_id_, video_width_, video_height_);
  }

  return texture_id_;
}

void AndroidMediaPlayer::SetSource(std::string_view source) {
  std::lock_guard<std::mutex> lock(mutex_);
  source_ = std::string(source);
}

std::string_view AndroidMediaPlayer::GetSource() const noexcept {
  return source_;
}

void AndroidMediaPlayer::Play() {
  if (media_player_ == nullptr) {
    // Create and configure MediaPlayer.
    JNIEnv* env = GetEnv();
    if (env == nullptr) {
      return;
    }

    jmethodID ctor = env->GetMethodID(media_player_class_, "<init>", "()V");
    jobject local_mp = env->NewObject(media_player_class_, ctor);
    if (local_mp == nullptr) {
      LOG(ERROR) << "AndroidMediaPlayer: failed to create MediaPlayer";
      return;
    }
    media_player_ = env->NewGlobalRef(local_mp);
    env->DeleteLocalRef(local_mp);

    // Set the Surface for video output.
    jmethodID set_surface = env->GetMethodID(
        media_player_class_, "setSurface",
        "(Landroid/view/Surface;)V");
    if (set_surface != nullptr && surface_ != nullptr) {
      env->CallVoidMethod(media_player_, set_surface, surface_);
    }

    // Set data source.
    jmethodID set_ds = env->GetMethodID(
        media_player_class_, "setDataSource",
        "(Ljava/lang/String;)V");
    if (set_ds != nullptr) {
      jstring path = env->NewStringUTF(source_.c_str());
      env->CallVoidMethod(media_player_, set_ds, path);
      env->DeleteLocalRef(path);
    }

    // Prepare async (OnPrepared callback will start playback).
    jmethodID prepare = env->GetMethodID(
        media_player_class_, "prepareAsync", "()V");
    if (prepare != nullptr) {
      env->CallVoidMethod(media_player_, prepare);
    }

    state_ = MediaState::kLoading;
    if (state_callback_ != nullptr) {
      state_callback_(state_);
    }
  } else {
    // Resume from pause.
    JNIEnv* env = GetEnv();
    if (env != nullptr) {
      jmethodID start = env->GetMethodID(
          media_player_class_, "start", "()V");
      if (start != nullptr) {
        env->CallVoidMethod(media_player_, start);
      }
    }
    state_ = MediaState::kPlaying;
    if (state_callback_ != nullptr) {
      state_callback_(state_);
    }
  }
}

void AndroidMediaPlayer::Pause() {
  if (media_player_ == nullptr) {
    return;
  }
  JNIEnv* env = GetEnv();
  if (env != nullptr) {
    jmethodID pause = env->GetMethodID(
        media_player_class_, "pause", "()V");
    if (pause != nullptr) {
      env->CallVoidMethod(media_player_, pause);
    }
  }
  state_ = MediaState::kPaused;
  if (state_callback_ != nullptr) {
    state_callback_(state_);
  }
}

void AndroidMediaPlayer::Stop() {
  if (media_player_ == nullptr) {
    return;
  }
  JNIEnv* env = GetEnv();
  if (env != nullptr) {
    jmethodID stop = env->GetMethodID(
        media_player_class_, "stop", "()V");
    if (stop != nullptr) {
      env->CallVoidMethod(media_player_, stop);
    }
    jmethodID reset = env->GetMethodID(
        media_player_class_, "reset", "()V");
    if (reset != nullptr) {
      env->CallVoidMethod(media_player_, reset);
    }
  }
  state_ = MediaState::kIdle;
  if (state_callback_ != nullptr) {
    state_callback_(state_);
  }
}

void AndroidMediaPlayer::Seek(double position_seconds) {
  if (media_player_ == nullptr) {
    return;
  }
  JNIEnv* env = GetEnv();
  if (env != nullptr) {
    jmethodID seek = env->GetMethodID(
        media_player_class_, "seekTo", "(I)V");
    if (seek != nullptr) {
      env->CallVoidMethod(media_player_, seek,
                          static_cast<jint>(position_seconds * 1000.0));
    }
  }
}

void AndroidMediaPlayer::SetVolume(double volume) {
  volume_ = volume;
  if (media_player_ == nullptr) {
    return;
  }
  JNIEnv* env = GetEnv();
  if (env != nullptr) {
    jmethodID set_vol = env->GetMethodID(
        media_player_class_, "setVolume", "(FF)V");
    if (set_vol != nullptr) {
      const jfloat v = static_cast<jfloat>(volume_);
      env->CallVoidMethod(media_player_, set_vol, v, v);
    }
  }
}

double AndroidMediaPlayer::GetVolume() const noexcept {
  return volume_;
}

double AndroidMediaPlayer::GetPosition() const noexcept {
  if (media_player_ == nullptr) {
    return 0.0;
  }
  JNIEnv* env = GetEnv();
  if (env == nullptr) {
    return 0.0;
  }
  jmethodID get_pos = env->GetMethodID(
      media_player_class_, "getCurrentPosition", "()I");
  if (get_pos == nullptr) {
    return 0.0;
  }
  const jint pos = env->CallIntMethod(media_player_, get_pos);
  return static_cast<double>(pos) / 1000.0;
}

double AndroidMediaPlayer::GetDuration() const noexcept {
  if (media_player_ == nullptr) {
    return 0.0;
  }
  JNIEnv* env = GetEnv();
  if (env == nullptr) {
    return 0.0;
  }
  jmethodID get_dur = env->GetMethodID(
      media_player_class_, "getDuration", "()I");
  if (get_dur == nullptr) {
    return 0.0;
  }
  const jint dur = env->CallIntMethod(media_player_, get_dur);
  return static_cast<double>(dur) / 1000.0;
}

MediaState AndroidMediaPlayer::GetState() const noexcept {
  return state_;
}

int AndroidMediaPlayer::GetVideoWidth() const noexcept {
  return video_width_;
}

int AndroidMediaPlayer::GetVideoHeight() const noexcept {
  return video_height_;
}

void AndroidMediaPlayer::SetStateCallback(StateCallback callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  state_callback_ = std::move(callback);
}

void AndroidMediaPlayer::SetFrameCallback(FrameCallback callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  frame_callback_ = std::move(callback);
}

void AndroidMediaPlayer::OnPrepared(int width, int height) {
  video_width_ = width;
  video_height_ = height;
  state_ = MediaState::kPlaying;
  if (media_player_ != nullptr) {
    JNIEnv* env = GetEnv();
    if (env != nullptr) {
      jmethodID start = env->GetMethodID(
          media_player_class_, "start", "()V");
      if (start != nullptr) {
        env->CallVoidMethod(media_player_, start);
      }
    }
  }
  if (state_callback_ != nullptr) {
    state_callback_(state_);
  }
}

void AndroidMediaPlayer::OnCompletion() {
  state_ = MediaState::kEnded;
  if (state_callback_ != nullptr) {
    state_callback_(state_);
  }
}

void AndroidMediaPlayer::OnError(int /*what*/, int /*extra*/) {
  state_ = MediaState::kError;
  if (state_callback_ != nullptr) {
    state_callback_(state_);
  }
}

void AndroidMediaPlayer::Release() {
  JNIEnv* env = GetEnv();
  if (env != nullptr) {
    if (media_player_ != nullptr) {
      jmethodID release = env->GetMethodID(
          media_player_class_, "release", "()V");
      if (release != nullptr) {
        env->CallVoidMethod(media_player_, release);
      }
      env->DeleteGlobalRef(media_player_);
      media_player_ = nullptr;
    }
    if (surface_ != nullptr) {
      jmethodID release = env->GetMethodID(
          env->GetObjectClass(surface_), "release", "()V");
      if (release != nullptr) {
        env->CallVoidMethod(surface_, release);
      }
      env->DeleteGlobalRef(surface_);
      surface_ = nullptr;
    }
    if (surface_texture_ != nullptr) {
      jmethodID release = env->GetMethodID(
          surface_texture_class_, "release", "()V");
      if (release != nullptr) {
        env->CallVoidMethod(surface_texture_, release);
      }
      env->DeleteGlobalRef(surface_texture_);
      surface_texture_ = nullptr;
    }
    if (media_player_class_ != nullptr) {
      env->DeleteGlobalRef(media_player_class_);
      media_player_class_ = nullptr;
    }
    if (surface_texture_class_ != nullptr) {
      env->DeleteGlobalRef(surface_texture_class_);
      surface_texture_class_ = nullptr;
    }
  }
  if (texture_id_ != 0) {
    glDeleteTextures(1, &texture_id_);
    texture_id_ = 0;
  }
}

}  // namespace neoflux

#endif  // ANDROID
