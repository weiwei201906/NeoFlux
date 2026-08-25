// =============================================================================
// NeoFlux - media_widget.cpp
//
// Integrated media playback widget. Uses the platform MediaPlayer backend to
// decode video frames into an OpenGL texture, then composites the texture into
// the widget's bounding rectangle via the render layer.
// =============================================================================

#include "neoflux/widget/media_widget.h"

#include <glog/logging.h>

#include <algorithm>

#include "neoflux/app/application.h"
#include "neoflux/media/media_player.h"
#include "neoflux/render/render_context.h"

namespace neoflux {

namespace {
// Default 16:9 aspect ratio for intrinsic sizing.
constexpr float kDefaultWidth = 480.0F;
constexpr float kDefaultHeight = 270.0F;
}  // namespace

MediaWidget::MediaWidget() {
  EnableMeasureFunction();
  player_ = CreateMediaPlayer();
  if (player_ != nullptr) {
    player_->SetStateCallback([this](MediaState state) {
      if (state == MediaState::kPlaying || state == MediaState::kPaused) {
        MarkNeedsBuild();
      }
    });
  }
}

MediaWidget::~MediaWidget() {
  if (player_ != nullptr) {
    player_->Stop();
  }
}

std::string_view MediaWidget::GetWidgetName() const noexcept {
  return "MediaWidget";
}

Size MediaWidget::OnMeasure(float /*width*/, int /*width_mode*/,
                            float /*height*/, int /*height_mode*/) {
  return Size{.width = kDefaultWidth, .height = kDefaultHeight};
}

void MediaWidget::Paint(RenderContext& context) {
  const Rect& b = GetBounds();
  if (b.width <= 0.0F || b.height <= 0.0F) {
    return;
  }

  // Initialize the player's render context on first paint (render thread).
  EnsurePlayerInit();

  // Update the video texture if a new frame is available.
  if (player_ != nullptr) {
    current_texture_ = player_->UpdateTexture();
    texture_width_ = player_->GetVideoWidth();
    texture_height_ = player_->GetVideoHeight();
  }

  // Draw placeholder background.
  context.DrawRoundedRect({.x = 0.0F, .y = 0.0F, .width = b.width,
                           .height = b.height,},
                          background_color_, 4.0F);

  // Draw the video texture if available.
  if (current_texture_ != 0) {
    context.DrawTexture(current_texture_,
                        {.x = 0.0F, .y = 0.0F, .width = b.width,
                         .height = b.height,});
  } else {
    // Draw placeholder text when no video frame is available.
    const char* msg = "No media loaded";
    if (player_ != nullptr && !player_->GetSource().empty()) {
      msg = "Loading...";
    }
    const float text_y = (b.height * 0.5F) + 6.0F;
    context.DrawText(msg, Point{.x = 12.0F, .y = text_y}, text_color_, 14.0F);
  }
}

bool MediaWidget::OnPointerDown(const Point& /*local_pos*/) {
  if (player_ == nullptr) {
    return false;
  }
  if (player_->GetState() == MediaState::kPlaying) {
    player_->Pause();
  } else {
    player_->Play();
  }
  return true;
}

void MediaWidget::SetSource(std::string_view source) {
  if (player_ != nullptr) {
    player_->SetSource(source);
  }
}

std::string_view MediaWidget::GetSource() const noexcept {
  if (player_ != nullptr) {
    return player_->GetSource();
  }
  return {};
}

void MediaWidget::Play() {
  if (player_ != nullptr) {
    player_->Play();
    MarkNeedsBuild();
  }
}

void MediaWidget::Pause() {
  if (player_ != nullptr) {
    player_->Pause();
  }
}

void MediaWidget::Stop() {
  if (player_ != nullptr) {
    player_->Stop();
    current_texture_ = 0;
  }
}

void MediaWidget::Seek(double position_seconds) {
  if (player_ != nullptr) {
    player_->Seek(position_seconds);
  }
}

void MediaWidget::SetVolume(double volume) {
  if (player_ != nullptr) {
    player_->SetVolume(volume);
  }
}

double MediaWidget::GetVolume() const noexcept {
  if (player_ != nullptr) {
    return player_->GetVolume();
  }
  return 0.0;
}

double MediaWidget::GetPosition() const noexcept {
  if (player_ != nullptr) {
    return player_->GetPosition();
  }
  return 0.0;
}

double MediaWidget::GetDuration() const noexcept {
  if (player_ != nullptr) {
    return player_->GetDuration();
  }
  return 0.0;
}

MediaState MediaWidget::GetState() const noexcept {
  if (player_ != nullptr) {
    return player_->GetState();
  }
  return MediaState::kIdle;
}

MediaPlayer* MediaWidget::GetPlayer() noexcept {
  return player_.get();
}

void MediaWidget::SetBackgroundColor(const Color& color) noexcept {
  background_color_ = color;
}

void MediaWidget::SetTextColor(const Color& color) noexcept {
  text_color_ = color;
}

void MediaWidget::EnsurePlayerInit() {
  if (player_ == nullptr || render_init_requested_) {
    return;
  }
  render_init_requested_ = true;
  player_->InitRender();
  LOG(INFO) << "MediaWidget: player render context initialized";
}

}  // namespace neoflux
