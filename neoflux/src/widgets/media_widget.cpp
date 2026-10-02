// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - media_widget.cpp
//
// Integrated media playback widget. Uses the platform MediaPlayer backend to
// decode video frames into an OpenGL texture, then composites the texture into
// the widget's bounding rectangle via the render layer.
//
// Threading:
//   - Paint() runs on the App/UI thread and only reads atomically-published
//     state (texture id, size). It performs NO GL calls.
//   - The actual mpv -> GL texture work (InitRender/UpdateTexture) runs on the
//     render thread via the RenderLayer render pump. When mpv decodes a new
//     frame on its internal thread, it signals RenderLayer::Wake() which wakes
//     the render thread to pull the frame, and MarkFrameDirty() which wakes the
//     App thread to repaint. mpv never pushes RenderCommands (SPSC preserved).
//
// Pimpl: MediaWidget::Impl owns the player handle and published texture state.
// =============================================================================

#include "neoflux/widgets/media_widget.h"

#include <glog/logging.h>

#include <algorithm>
#include <atomic>
#include <utility>

#include "neoflux/apps/application.h"
#include "neoflux/media/media_player.h"
#include "neoflux/renderers/render_context.h"
#include "neoflux/renderers/render_layer.h"

namespace neoflux {

namespace {
// Default 16:9 aspect ratio for intrinsic sizing.
constexpr float kDefaultWidth = 480.0F;
constexpr float kDefaultHeight = 270.0F;
}  // namespace

struct MediaWidget::Impl {
  // Runs on the render thread (GL context current). Initializes the player's
  // render context on first call, then pulls the newest mpv frame into the GL
  // texture and publishes the result to the atomics read by Paint().
  void PumpOnRenderThread() {
    if (player == nullptr) {
      return;
    }
    if (!render_init_requested) {
      player->InitRender();
      render_init_requested = true;
      LOG(INFO) << "MediaWidget: player render context initialized";
    }
    const std::uint32_t tex = player->UpdateTexture();
    published_texture.store(tex);
    published_w.store(player->GetVideoWidth());
    published_h.store(player->GetVideoHeight());
  }

  std::unique_ptr<MediaPlayer> player;
  bool render_init_requested = false;

  // Atomically-published frame state. Written by PumpOnRenderThread on the
  // render thread, read by Paint on the App thread.
  std::atomic<std::uint32_t> published_texture{0};
  std::atomic<int> published_w{0};
  std::atomic<int> published_h{0};

  // Wiring captured on the first Build() (App thread). render_layer is owned by
  // the Application and outlives this widget (the render thread is joined in
  // Application::Stop before the widget tree is destroyed).
  Application* app = nullptr;
  RenderLayer* render_layer = nullptr;
  bool wired = false;

  Color background_color{.r = 20, .g = 20, .b = 20, .a = 255};
  Color text_color{.r = 255, .g = 255, .b = 255, .a = 255};
};

MediaWidget::MediaWidget() : impl_(std::make_unique<Impl>()) {
  EnableMeasureFunction();
  impl_->player = CreateMediaPlayer();
  if (impl_->player != nullptr) {
    // State changes (kLoading/kPlaying/kPaused/kEnded) do NOT trigger a widget
    // rebuild: Paint() reads the atomically-published texture every frame and
    // the transport buttons live in the user's view, not here. Rebuilding on
    // every state transition caused an infinite loop (rebuild -> new MediaWidget
    // -> Play() -> loadfile -> FILE_LOADED -> state change -> rebuild).
    impl_->player->SetStateCallback([](MediaState /*state*/) {
      // Intentionally empty: texture is atomic-published, no tree rebuild needed.
    });
  }
}

MediaWidget::~MediaWidget() {
  // Teardown ordering (Application::Stop clears navigation_stack_ BEFORE it
  // joins the render thread, so the render thread still owns the current GL
  // context when this runs):
  //   1. Detach the per-frame pump so the render thread stops pulling mpv frames.
  //   2. Detach mpv's internal-thread wake callback and issue mpv stop.
  //   3. Free the mpv render context + GL texture/FBO ON THE RENDER THREAD (the
  //      only thread with the GL context current). RunOnRenderThread blocks until
  //      that completes, so by the time this destructor returns and the player
  //      unique_ptr runs ~MpvMediaPlayer, render_ctx is already nullptr and the
  //      App-thread destructor only does mpv_terminate_destroy (no GL calls).
  if (impl_->render_layer != nullptr) {
    impl_->render_layer->SetRenderPump(nullptr);
  }
  if (impl_->player != nullptr) {
    impl_->player->SetWakeCallback(nullptr);
    impl_->player->Stop();
    if (impl_->render_layer != nullptr) {
      // Non-owning observer only: the player unique_ptr below outlives this
      // synchronous call (RunOnRenderThread blocks until the lambda returns).
      MediaPlayer* player = impl_->player.get();
      impl_->render_layer->RunOnRenderThread(
          [player]() { player->TeardownRender(); });
    }
  }
}

std::string_view MediaWidget::GetWidgetName() const noexcept {
  return "MediaWidget";
}

Size MediaWidget::OnMeasure(float /*width*/, int /*width_mode*/,
                            float /*height*/, int /*height_mode*/) {
  return Size{.width = kDefaultWidth, .height = kDefaultHeight};
}

std::shared_ptr<Widget> MediaWidget::Build(BuildContext& context) {
  // Wire the mpv frame signal to the render thread exactly once, on the App
  // thread during the first build.
  if (!impl_->wired) {
    impl_->wired = true;
    impl_->app = context.GetApplication();
    if (impl_->app != nullptr) {
      impl_->render_layer = &impl_->app->GetRenderLayer();
    }
    if (impl_->player != nullptr && impl_->render_layer != nullptr) {
      // Render thread: pull mpv frames into a GL texture.
      impl_->render_layer->SetRenderPump(
          [this]() { impl_->PumpOnRenderThread(); });
      // mpv internal thread: a new frame is decoded -> wake the render thread
      // to upload it AND wake the App thread to repaint. Both are non-blocking
      // and touch no GL.
      MediaPlayer* player = impl_->player.get();
      Application* app = impl_->app;
      RenderLayer* layer = impl_->render_layer;
      player->SetWakeCallback([app, layer]() {
        if (layer != nullptr) {
          layer->Wake();
        }
        if (app != nullptr) {
          app->MarkFrameDirty();
        }
      });
    }
  }
  return nullptr;
}

void MediaWidget::Paint(RenderContext& context) {
  const Rect& b = GetBounds();
  if (b.width <= 0.0F || b.height <= 0.0F) {
    return;
  }

  // Read the atomically-published texture. GL upload happens on the render
  // thread (the pump); this App-thread method only emits a DrawTexture command.
  const std::uint32_t texture = impl_->published_texture.load();

  // Draw placeholder background.
  context.DrawRoundedRect({.x = 0.0F, .y = 0.0F, .width = b.width,
                           .height = b.height,},
                          impl_->background_color, 4.0F);

  // Draw the video texture if available.
  if (texture != 0U) {
    context.DrawTexture(texture,
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

MediaWidget& MediaWidget::SetSource(std::string_view source) {
  if (impl_->player != nullptr) {
    impl_->player->SetSource(source);
  }
  return *this;
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
    impl_->published_texture.store(0);
  }
}

void MediaWidget::Seek(double position_seconds) {
  if (impl_->player != nullptr) {
    impl_->player->Seek(position_seconds);
  }
}

MediaWidget& MediaWidget::SetVolume(double volume) {
  if (impl_->player != nullptr) {
    impl_->player->SetVolume(volume);
  }
  return *this;
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

MediaWidget& MediaWidget::SetBackgroundColor(const Color& color) noexcept {
  impl_->background_color = color;
  return *this;
}

MediaWidget& MediaWidget::SetTextColor(const Color& color) noexcept {
  impl_->text_color = color;
  return *this;
}

}  // namespace neoflux
