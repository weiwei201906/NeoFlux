// =============================================================================
// NeoFlux - media_widget.cpp
//
// Integrated media playback widget. Uses the platform MediaPlayer backend to
// decode video frames into an OpenGL texture, then composites the texture into
// the widget's bounding rectangle via the render layer.
//
// Pimpl: MediaWidget::Impl owns the player handle and current GL texture.
// =============================================================================

#include "neoflux/widgets/media_widget.h"

#include <glog/logging.h>

#include <algorithm>
#include <utility>

#include "neoflux/apps/application.h"
#include "neoflux/media/media_player.h"
#include "neoflux/renderers/render_context.h"

namespace neoflux {

namespace {
// Default 16:9 aspect ratio for intrinsic sizing.
constexpr float kDefaultWidth = 480.0F;
constexpr float kDefaultHeight = 270.0F;
}  // namespace

struct MediaWidget::Impl {
  // Initializes the media player render context on the render thread.
  void EnsurePlayerInit() {
    if (player == nullptr || render_init_requested) {
      return;
    }
    render_init_requested = true;
    player->InitRender();
    LOG(INFO) << "MediaWidget: player render context initialized";
  }

  std::unique_ptr<MediaPlayer> player;
  bool render_init_requested = false;
  std::uint32_t current_texture = 0;
  int texture_width = 0;
  int texture_height = 0;

  Color background_color{.r = 20, .g = 20, .b = 20, .a = 255};
  Color text_color{.r = 255, .g = 255, .b = 255, .a = 255};
};

MediaWidget::MediaWidget() : impl_(std::make_unique<Impl>()) {
  EnableMeasureFunction();
  impl_->player = CreateMediaPlayer();
  if (impl_->player != nullptr) {
    impl_->player->SetStateCallback([this](MediaState state) {
      if (state == MediaState::kPlaying || state == MediaState::kPaused) {
        MarkNeedsBuild();
      }
    });
  }
}

MediaWidget::~MediaWidget() {
  if (impl_->player != nullptr) {
    impl_->player->Stop();
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
  impl_->EnsurePlayerInit();

  // Update the video texture if a new frame is available.
  if (impl_->player != nullptr) {
    impl_->current_texture = impl_->player->UpdateTexture();
    impl_->texture_width = impl_->player->GetVideoWidth();
    impl_->texture_height = impl_->player->GetVideoHeight();
  }

  // Draw placeholder background.
  context.DrawRoundedRect({.x = 0.0F, .y = 0.0F, .width = b.width,
                           .height = b.height,},
                          impl_->background_color, 4.0F);

  // Draw the video texture if available.
  if (impl_->current_texture != 0) {
    context.DrawTexture(impl_->current_texture,
                        {.x = 0.0F, .y = 0.0F, .width = b.width,
                         .height = b.height,});
  } else {
    // Draw placeholder text when no video frame is available.
    const char* msg = "No media loaded";
    if (impl_->player != nullptr && !impl_->player->GetSource().empty()) {
      msg = "Loading...";
    }
    const float text_y = (b.height * 0.5F) + 6.0F;
    context.DrawText(msg, Point{.x = 12.0F, .y = text_y}, impl_->text_color,
                     14.0F);
  }
}

bool MediaWidget::OnPointerDown(const Point& /*local_pos*/) {
  if (impl_->player == nullptr) {
    return false;
  }
  if (impl_->player->GetState() == MediaState::kPlaying) {
    impl_->player->Pause();
  } else {
    impl_->player->Play();
  }
  return true;
}

void MediaWidget::SetSource(std::string_view source) {
  if (impl_->player != nullptr) {
    impl_->player->SetSource(source);
  }
}

std::string_view MediaWidget::GetSource() const noexcept {
  if (impl_->player != nullptr) {
    return impl_->player->GetSource();
  }
  return {};
}

void MediaWidget::Play() {
  if (impl_->player != nullptr) {
    impl_->player->Play();
    MarkNeedsBuild();
  }
}

void MediaWidget::Pause() {
  if (impl_->player != nullptr) {
    impl_->player->Pause();
  }
}

void MediaWidget::Stop() {
  if (impl_->player != nullptr) {
    impl_->player->Stop();
    impl_->current_texture = 0;
  }
}

void MediaWidget::Seek(double position_seconds) {
  if (impl_->player != nullptr) {
    impl_->player->Seek(position_seconds);
  }
}

void MediaWidget::SetVolume(double volume) {
  if (impl_->player != nullptr) {
    impl_->player->SetVolume(volume);
  }
}

double MediaWidget::GetVolume() const noexcept {
  if (impl_->player != nullptr) {
    return impl_->player->GetVolume();
  }
  return 0.0;
}

double MediaWidget::GetPosition() const noexcept {
  if (impl_->player != nullptr) {
    return impl_->player->GetPosition();
  }
  return 0.0;
}

double MediaWidget::GetDuration() const noexcept {
  if (impl_->player != nullptr) {
    return impl_->player->GetDuration();
  }
  return 0.0;
}

MediaState MediaWidget::GetState() const noexcept {
  if (impl_->player != nullptr) {
    return impl_->player->GetState();
  }
  return MediaState::kIdle;
}

MediaPlayer* MediaWidget::GetPlayer() noexcept {
  return impl_->player.get();
}

void MediaWidget::SetBackgroundColor(const Color& color) noexcept {
  impl_->background_color = color;
}

void MediaWidget::SetTextColor(const Color& color) noexcept {
  impl_->text_color = color;
}

}  // namespace neoflux
