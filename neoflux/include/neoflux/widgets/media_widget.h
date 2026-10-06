// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - media_widget.h
//
// Integrated media playback widget. Uses the platform MediaPlayer backend
// (libmpv on desktop, native players on mobile) to decode video frames into a
// CPU image that is composited into the widget tree.
//
// This is a texture-sharing-style media player without the texture: video
// decoding happens in the platform backend, the decoded frame becomes a tgfx
// image published under an opaque id, and the NeoFlux render layer composites
// that image into the widget's bounding rectangle through the active tgfx
// backend. NeoFlux owns no GPU object and makes no GL call.
//
// Pimpl: the player handle and the published frame state live in
// MediaWidget::Impl, defined in the .cpp, so this header leaks no mpv/GL types.
//
// All method implementations are in src/widget/media_widget.cpp.
// =============================================================================

#ifndef NEOFLUX_WIDGET_MEDIA_WIDGET_H_
#define NEOFLUX_WIDGET_MEDIA_WIDGET_H_

#include <memory>
#include <string_view>

#include "neoflux/media/media_player.h"
#include "neoflux/widgets/widget.h"

namespace neoflux {

// Media playback widget with integrated video rendering.
//
// Usage:
//   auto media = std::make_shared<MediaWidget>();
//   media->SetSource("video.mp4");
//   media->Play();
//   container->AddChild(media);
//
// The widget composites the decoded video texture into its bounding rectangle.
// Playback controls (play/pause/seek/volume) are exposed via methods; build
// your own control UI on top (buttons, sliders) or use the built-in tap-to-
// toggle behavior.
class MediaWidget : public Widget {
 public:
  MediaWidget();
  // App/UI thread. Runs during navigation_stack_ teardown, BEFORE the render
  // thread is joined. It synchronously offloads the player's render teardown
  // (mpv render context, published frame image, CPU frame buffers) to the render
  // thread that drives the frame pump, and blocks until that completes; only
  // then does it let the player unique_ptr tear down the remaining mpv core. No
  // GL context is involved. Do not call it after RenderLayer::Stop().
  ~MediaWidget() override;

  // Returns the human-readable widget name for debugging.
  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;

  // Reports intrinsic media widget size to the Taitank layout engine.
  [[nodiscard]] Size OnMeasure(float width, int width_mode, float height,
                               int height_mode) override;

  // On the first build, captures the owning Application/RenderLayer and wires
  // the frame signal to the render thread. Returns nullptr (leaf widget).
  [[nodiscard]] std::shared_ptr<Widget> Build(BuildContext& context) override;

  // Paints the decoded video frame (if available) or a placeholder surface.
  // Runs on the App/UI thread; reads an atomically-published frame-image id and
  // never calls GL.
  void Paint(RenderContext& context) override;

  // Toggles play/pause on click.
  bool OnPointerDown(const Point& local_pos) override;

  // --- Playback controls ---

  // Sets the media source (file path or URL).
  MediaWidget& SetSource(std::string_view source);

  // Returns the current media source.
  [[nodiscard]] std::string_view GetSource() const noexcept;

  // Starts playback.
  void Play();

  // Pauses playback.
  void Pause();

  // Stops playback and resets to beginning.
  void Stop();

  // Seeks to the given position in seconds.
  void Seek(double position_seconds);

  // Sets volume (0.0 = mute, 1.0 = full).
  MediaWidget& SetVolume(double volume);

  // Returns current volume.
  [[nodiscard]] double GetVolume() const noexcept;

  // Returns current playback position in seconds.
  [[nodiscard]] double GetPosition() const noexcept;

  // Returns total duration in seconds.
  [[nodiscard]] double GetDuration() const noexcept;

  // Returns current playback state.
  [[nodiscard]] MediaState GetState() const noexcept;

  // Returns the underlying media player (for advanced control).
  [[nodiscard]] MediaPlayer* GetPlayer() noexcept;

  // --- Appearance ---

  // Sets the placeholder background color (shown before first frame).
  MediaWidget& SetBackgroundColor(const Color& color) noexcept;

  // Sets the placeholder text color.
  MediaWidget& SetTextColor(const Color& color) noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_MEDIA_WIDGET_H_
