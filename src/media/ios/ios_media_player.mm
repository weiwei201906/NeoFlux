// =============================================================================
// NeoFlux - ios_media_player.mm
//
// iOS media player implementation using AVPlayer + CVPixelBuffer +
// CVOpenGLESTextureCache. Video frames are converted to OpenGL ES textures
// for compositing by the NeoFlux render layer.
//
// Compiled only on iOS (TARGET_OS_IPHONE). Requires AVFoundation and
// CoreVideo frameworks.
// =============================================================================

#include "neoflux/media/ios/ios_media_player.h"

#if defined(__APPLE__) && defined(TARGET_OS_IPHONE)

#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>
#import <QuartzCore/QuartzCore.h>

#include <glog/logging.h>

#include <mutex>

namespace neoflux {

IosMediaPlayer::IosMediaPlayer() = default;

IosMediaPlayer::~IosMediaPlayer() {
  Stop();
  if (texture_cache_ != nullptr) {
    CFRelease(texture_cache_);
    texture_cache_ = nullptr;
  }
  if (texture_id_ != 0) {
    glDeleteTextures(1, &texture_id_);
    texture_id_ = 0;
  }
}

bool IosMediaPlayer::CreateTextureCache() {
  EAGLContext* context = [EAGLContext currentContext];
  if (context == nullptr) {
    LOG(ERROR) << "IosMediaPlayer: no current EAGL context";
    return false;
  }

  CVReturn ret = CVOpenGLESTextureCacheCreate(
      kCFAllocatorDefault, nullptr, context, nullptr,
      reinterpret_cast<CVOpenGLESTextureCacheRef*>(&texture_cache_));
  if (ret != kCVReturnSuccess) {
    LOG(ERROR) << "IosMediaPlayer: CVOpenGLESTextureCacheCreate failed: " << ret;
    return false;
  }

  render_initialized_ = true;
  LOG(INFO) << "IosMediaPlayer: texture cache created";
  return true;
}

void IosMediaPlayer::InitRender() {
  if (render_initialized_) {
    return;
  }
  CreateTextureCache();
}

std::uint32_t IosMediaPlayer::UpdateTexture() {
  if (video_output_ == nullptr || texture_cache_ == nullptr) {
    return 0;
  }
  return UpdateTextureFromPixelBuffer();
}

std::uint32_t IosMediaPlayer::UpdateTextureFromPixelBuffer() {
  AVPlayerItemVideoOutput* output =
      static_cast<AVPlayerItemVideoOutput*>(video_output_);
  if (output == nullptr) {
    return 0;
  }

  CMTime item_time = [output itemTimeForHostTime:CACurrentMediaTime()];
  if (![output hasNewPixelBufferForItemTime:item_time]) {
    return texture_id_;
  }

  CVPixelBufferRef pixel_buffer =
      [output copyPixelBufferForItemTime:item_time itemTimeForDisplay:nil];
  if (pixel_buffer == nullptr) {
    return texture_id_;
  }

  const size_t width = CVPixelBufferGetWidth(pixel_buffer);
  const size_t height = CVPixelBufferGetHeight(pixel_buffer);
  texture_width_ = static_cast<int>(width);
  texture_height_ = static_cast<int>(height);

  // Create or reuse the GL texture from the pixel buffer.
  CVOpenGLESTextureRef gl_texture = nullptr;
  CVReturn ret = CVOpenGLESTextureCacheCreateTextureFromImage(
      kCFAllocatorDefault,
      static_cast<CVOpenGLESTextureCacheRef>(texture_cache_),
      pixel_buffer, nullptr, GL_TEXTURE_2D, GL_RGBA,
      static_cast<size_t>(width), static_cast<size_t>(height),
      GL_BGRA, GL_UNSIGNED_BYTE, 0, &gl_texture);

  if (ret == kCVReturnSuccess && gl_texture != nullptr) {
    texture_id_ = CVOpenGLESTextureGetName(gl_texture);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CFRelease(gl_texture);

    if (frame_callback_ != nullptr) {
      frame_callback_(texture_id_, texture_width_, texture_height_);
    }
  }

  CVPixelBufferRelease(pixel_buffer);
  return texture_id_;
}

void IosMediaPlayer::SetSource(std::string_view source) {
  std::lock_guard<std::mutex> lock(mutex_);
  source_ = std::string(source);
}

std::string_view IosMediaPlayer::GetSource() const noexcept {
  return source_;
}

void IosMediaPlayer::Play() {
  if (player_ == nullptr) {
    // Create AVPlayer with the source URL.
    NSString* url_str = [NSString stringWithUTF8String:source_.c_str()];
    NSURL* url = [NSURL URLWithString:url_str];
    if (url == nullptr) {
      // Try as file path.
      url = [NSURL fileURLWithPath:url_str];
    }

    AVPlayer* player = [AVPlayer playerWithURL:url];
    player.volume = static_cast<float>(volume_);
    player_ = (__bridge_retained void*)player;

    // Create video output for pixel buffer access.
    NSDictionary* pix_buffer_attrs = @{
      (id)kCVPixelBufferPixelFormatTypeKey:
          @(kCVPixelFormatType_32BGRA),
    };
    AVPlayerItemVideoOutput* output =
        [[AVPlayerItemVideoOutput alloc] initWithPixelBufferAttributes:pix_buffer_attrs];
    video_output_ = (__bridge_retained void*)output;

    AVPlayerItem* item = player.currentItem;
    if (item != nullptr) {
      [item addOutput:output];
    }

    state_ = MediaState::kLoading;
    if (state_callback_ != nullptr) {
      state_callback_(state_);
    }

    // Observe status changes via KVO (simplified: poll in UpdateTexture).
    [player play];
    state_ = MediaState::kPlaying;
    if (state_callback_ != nullptr) {
      state_callback_(state_);
    }
  } else {
    AVPlayer* player = (__bridge AVPlayer*)player_;
    [player play];
    state_ = MediaState::kPlaying;
    if (state_callback_ != nullptr) {
      state_callback_(state_);
    }
  }
}

void IosMediaPlayer::Pause() {
  if (player_ == nullptr) {
    return;
  }
  AVPlayer* player = (__bridge AVPlayer*)player_;
  [player pause];
  state_ = MediaState::kPaused;
  if (state_callback_ != nullptr) {
    state_callback_(state_);
  }
}

void IosMediaPlayer::Stop() {
  if (player_ != nullptr) {
    AVPlayer* player = (__bridge AVPlayer*)player_;
    [player pause];
    [player replaceCurrentItemWithPlayerItem:nil];
    player_ = nullptr;  // ARC releases
  }
  if (video_output_ != nullptr) {
    video_output_ = nullptr;
  }
  state_ = MediaState::kIdle;
  if (state_callback_ != nullptr) {
    state_callback_(state_);
  }
}

void IosMediaPlayer::Seek(double position_seconds) {
  if (player_ == nullptr) {
    return;
  }
  AVPlayer* player = (__bridge AVPlayer*)player_;
  CMTime time = CMTimeMakeWithSeconds(position_seconds, 600);
  [player seekToTime:time];
}

void IosMediaPlayer::SetVolume(double volume) {
  volume_ = volume;
  if (player_ != nullptr) {
    AVPlayer* player = (__bridge AVPlayer*)player_;
    player.volume = static_cast<float>(volume_);
  }
}

double IosMediaPlayer::GetVolume() const noexcept {
  return volume_;
}

double IosMediaPlayer::GetPosition() const noexcept {
  if (player_ == nullptr) {
    return 0.0;
  }
  AVPlayer* player = (__bridge AVPlayer*)player_;
  return CMTimeGetSeconds(player.currentTime);
}

double IosMediaPlayer::GetDuration() const noexcept {
  if (player_ == nullptr) {
    return 0.0;
  }
  AVPlayer* player = (__bridge AVPlayer*)player_;
  AVPlayerItem* item = player.currentItem;
  if (item == nullptr) {
    return 0.0;
  }
  return CMTimeGetSeconds(item.duration);
}

MediaState IosMediaPlayer::GetState() const noexcept {
  return state_;
}

int IosMediaPlayer::GetVideoWidth() const noexcept {
  return texture_width_;
}

int IosMediaPlayer::GetVideoHeight() const noexcept {
  return texture_height_;
}

void IosMediaPlayer::SetStateCallback(StateCallback callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  state_callback_ = std::move(callback);
}

void IosMediaPlayer::SetFrameCallback(FrameCallback callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  frame_callback_ = std::move(callback);
}

}  // namespace neoflux

#endif  // __APPLE__ && TARGET_OS_IPHONE
