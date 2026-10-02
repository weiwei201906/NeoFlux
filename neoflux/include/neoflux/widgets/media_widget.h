// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - media_widget.h
//
// Integrated media playback widget. Uses the platform MediaPlayer backend
// (libmpv on desktop, native players on mobile) to decode video frames into
// an OpenGL texture that is composited directly into the widget tree.
//
// This is a Flutter-style texture-sharing media player: video decoding happens
// in the platform backend, and frames are rendered to a GL texture that the
// NeoFlux render layer composites into the widget's bounding rectangle.
//
// Pimpl: render/GL state (the player handle and current texture) lives in
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
  // thread is joined. It synchronously offloads the mpv render-context and GL
  // texture/FBO destruction to the render thread (which owns the GL context) and
  // blocks until that completes; only then does it let the player unique_ptr
  // tear down the non-GL mpv core. Do not call it after RenderLayer::Stop().
  ~MediaWidget() override;

  // Returns the human-readable widget name for debugging.
  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;

  // Reports intrinsic media widget size to the Taitank layout engine.
  [[nodiscard]] Size OnMeasure(float width, int width_mode, float height,
                               int height_mode) override;

  // On the first build, captures the owning Application/RenderLayer and wires
  // the mpv frame signal to the render thread. Returns nullptr (leaf widget).
  [[nodiscard]] std::shared_ptr<Widget> Build(BuildContext& context) override;

  // Paints the video texture (if available) or a placeholder surface. Runs on
  // the App/UI thread; reads an atomically-published texture id and never calls
  // GL directly.
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
