// =============================================================================
// NeoFlux - media_widget.h
//
// Media playback widget backed by the ffplay subprocess (from FFmpeg).
// Launches ffplay as a child process to play audio/video files. The widget
// itself renders a placeholder surface with play/pause controls and media
// metadata; actual video decoding and rendering is delegated to ffplay.
//
// ffplay must be available on the system PATH. On Windows, download FFmpeg
// from https://ffmpeg.org/download.html and add the bin/ directory to PATH.
//
// All method implementations are in src/widget/media_widget.cpp.
// =============================================================================

#ifndef NEOFLUX_WIDGET_MEDIA_WIDGET_H_
#define NEOFLUX_WIDGET_MEDIA_WIDGET_H_

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "neoflux/widget/widget.h"

namespace neoflux {

// Media playback state.
enum class MediaState : std::uint8_t {
  kStopped = 0,
  kPlaying = 1,
  kPaused = 2,
};

// Media player widget that delegates playback to an ffplay subprocess.
//
// Usage:
//   auto media = std::make_shared<MediaWidget>();
//   media->SetSource("video.mp4");
//   container->AddChild(media);
//
// The widget renders a placeholder surface with a play button overlay.
// Clicking the play button launches ffplay in a separate window. On desktop
// platforms, ffplay creates its own SDL window for video output.
class MediaWidget : public Widget {
 public:
  MediaWidget();
  ~MediaWidget() override;

  // Returns the human-readable widget name for debugging.
  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;

  // Paints the media widget: placeholder surface, play/pause button,
  // media filename, and playback state indicator.
  // Reports intrinsic media widget size to the Taitank layout engine.
  [[nodiscard]] Size OnMeasure(float width, int width_mode, float height,
                               int height_mode) override;

  void Paint(RenderContext& context) override;

  // Handles pointer press: toggles play/pause when clicking the button area.
  bool OnPointerDown(const Point& local_pos) override;

  // --- Configuration ---

  // Sets the media file path (local file or URL supported by ffplay).
  void SetSource(std::string_view source);

  // Returns the current media source.
  [[nodiscard]] std::string_view GetSource() const noexcept;

  // Sets the ffplay executable path. Defaults to "ffplay" (on PATH).
  void SetFfplayPath(std::string_view path);

  // Sets extra command-line arguments passed to ffplay (e.g. "-vcodec h264
  // -acodec aac -fs"). These are appended after the default "-autoexit" flag.
  void SetExtraArgs(std::string_view args);

  // Sets whether ffplay should auto-play when SetSource is called.
  void SetAutoPlay(bool auto_play) noexcept;

  // Starts playback (launches ffplay subprocess).
  void Play();

  // Pauses playback (terminates ffplay subprocess; ffplay has no pause IPC).
  void Pause();

  // Stops playback (terminates ffplay subprocess).
  void Stop();

  // Returns the current playback state.
  [[nodiscard]] MediaState GetState() const noexcept;

  // Sets the background color of the placeholder surface.
  void SetBackgroundColor(const Color& color) noexcept;

  // Sets the play button color.
  void SetButtonColor(const Color& color) noexcept;

  // Sets the text color for the filename label.
  void SetTextColor(const Color& color) noexcept;

 private:
  // Launches the ffplay subprocess with the current source.
  void LaunchFfplay();

  // Terminates the ffplay subprocess if running.
  void TerminateFfplay();

  // Returns true if the point is within the play button area.
  [[nodiscard]] bool HitPlayButton(const Point& local_pos) const noexcept;

  std::string source_{};
  std::string ffplay_path_{"ffplay"};
  std::string extra_args_{};
  MediaState state_ = MediaState::kStopped;
  bool auto_play_ = false;

  // Opaque handle to the child process. On Windows this is a PROCESS_INFORMATION
  // pointer; on POSIX this is a pid_t. Stored as void* to avoid platform
  // headers in the public API.
  void* process_handle_ = nullptr;

  Color background_color_{.r = 20, .g = 20, .b = 20, .a = 255};
  Color button_color_{.r = 66, .g = 133, .b = 244, .a = 255};
  Color text_color_{.r = 255, .g = 255, .b = 255, .a = 255};
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_MEDIA_WIDGET_H_
