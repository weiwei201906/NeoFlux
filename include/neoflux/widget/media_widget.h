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
// All method implementations are in src/widget/media_widget.cpp.
// =============================================================================

#ifndef NEOFLUX_WIDGET_MEDIA_WIDGET_H_
#define NEOFLUX_WIDGET_MEDIA_WIDGET_H_

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "neoflux/media/media_player.h"
#include "neoflux/widget/widget.h"

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
  ~MediaWidget() override;

  // Returns the human-readable widget name for debugging.
  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;

  // Reports intrinsic media widget size to the Taitank layout engine.
  [[nodiscard]] Size OnMeasure(float width, int width_mode, float height,
                               int height_mode) override;

  // Paints the video texture (if available) or a placeholder surface.
  void Paint(RenderContext& context) override;

  // Toggles play/pause on click.
  bool OnPointerDown(const Point& local_pos) override;

  // --- Playback controls ---

  // Sets the media source (file path or URL).
  void SetSource(std::string_view source);

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
  void SetVolume(double volume);

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
  void SetBackgroundColor(const Color& color) noexcept;

  // Sets the placeholder text color.
  void SetTextColor(const Color& color) noexcept;

 private:
  // Initializes the media player render context on the render thread.
  void EnsurePlayerInit();

  std::unique_ptr<MediaPlayer> player_{};
  bool render_init_requested_ = false;
  std::uint32_t current_texture_ = 0;
  int texture_width_ = 0;
  int texture_height_ = 0;

  Color background_color_{.r = 20, .g = 20, .b = 20, .a = 255};
  Color text_color_{.r = 255, .g = 255, .b = 255, .a = 255};
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_MEDIA_WIDGET_H_
