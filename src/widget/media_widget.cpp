// =============================================================================
// NeoFlux - media_widget.cpp
//
// Implementation of MediaWidget. Uses ffplay as an external subprocess for
// media decoding and rendering. Child process management is platform-specific:
//   Windows: CreateProcess / TerminateProcess
//   POSIX:   fork/execvp / kill
// =============================================================================

#include "neoflux/widget/media_widget.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <sstream>
#include <utility>
#include <vector>

#include <glog/logging.h>

#include "neoflux/render/render_context.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
// windows.h defines DrawText as a macro (DrawTextA/DrawTextW), which conflicts
// with RenderContext::DrawText. Undefine it after including windows.h.
#undef DrawText
#undef GetMessage
#undef SendMessage
#else
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace neoflux {

namespace {

// Platform-specific process handle wrapper.
#ifdef _WIN32
struct ProcessHandle {
  PROCESS_INFORMATION pi{};
  bool valid = false;
};
#else
struct ProcessHandle {
  pid_t pid = -1;
  bool valid = false;
};
#endif

}  // namespace

MediaWidget::MediaWidget() {
  EnableMeasureFunction();
#ifdef NEOFLUX_FFPLAY_PATH
  ffplay_path_ = NEOFLUX_FFPLAY_PATH;
#endif
}

MediaWidget::~MediaWidget() { Stop(); }

std::string_view MediaWidget::GetWidgetName() const noexcept {
  return "MediaWidget";
}

Size MediaWidget::OnMeasure(float /*width*/, int /*width_mode*/,
                            float /*height*/, int /*height_mode*/) {
  // Default 16:9 aspect ratio intrinsic size.
  return Size{.width = 480.0F, .height = 270.0F};
}

void MediaWidget::Paint(RenderContext& context) {
  const Rect& b = GetBounds();
  if (b.width <= 0.0F || b.height <= 0.0F) {
    return;
  }

  // Background surface.
  context.DrawRoundedRect(b, background_color_, 8.0F);

  // Play/pause button (centered circle).
  const float btn_size = std::min(b.width, b.height) * 0.25F;
  const float btn_x = b.x + ((b.width - btn_size) * 0.5F);
  const float btn_y = b.y + ((b.height - btn_size) * 0.5F);
  const Rect btn_rect{.x = btn_x,
                      .y = btn_y,
                      .width = btn_size,
                      .height = btn_size,};
  context.DrawRoundedRect(btn_rect, button_color_, btn_size * 0.5F);

  // Play triangle or pause bars inside the button.
  const float cx = btn_x + (btn_size * 0.5F);
  const float cy = btn_y + (btn_size * 0.5F);
  const float tri = btn_size * 0.3F;
  if (state_ == MediaState::kPlaying) {
    // Pause icon: two vertical bars.
    const float bar_w = btn_size * 0.1F;
    const float bar_h = btn_size * 0.35F;
    const Rect bar1{.x = cx - (tri * 0.5F) - (bar_w * 0.5F),
                    .y = cy - (bar_h * 0.5F),
                    .width = bar_w,
                    .height = bar_h,};
    const Rect bar2{.x = cx + (tri * 0.5F) - (bar_w * 0.5F),
                    .y = cy - (bar_h * 0.5F),
                    .width = bar_w,
                    .height = bar_h,};
    context.DrawRect(bar1, text_color_);
    context.DrawRect(bar2, text_color_);
  } else {
    // Play icon: triangle (approximated by a small rect rotated is complex;
    // use three rects to form a right-pointing triangle).
    const Rect tri_body{.x = cx - (tri * 0.2F),
                        .y = cy - (tri * 0.5F),
                        .width = tri * 0.4F,
                        .height = tri,};
    context.DrawRect(tri_body, text_color_);
  }

  // Filename label at the bottom.
  if (!source_.empty()) {
    const float label_y = b.y + b.height - 24.0F;
    // Extract filename from path.
    std::size_t slash = source_.find_last_of("/\\");
    std::string_view filename =
        (slash != std::string_view::npos)
            ? std::string_view(source_).substr(slash + 1)
            : std::string_view(source_);
    context.DrawText(filename, Point{.x = b.x + 8.0F, .y = label_y},
                     text_color_, 12.0F);
  }
}

bool MediaWidget::OnPointerDown(const Point& local_pos) {
  if (HitPlayButton(local_pos)) {
    if (state_ == MediaState::kPlaying) {
      Pause();
    } else {
      Play();
    }
    return true;
  }
  return false;
}

void MediaWidget::SetSource(std::string_view source) {
  source_ = std::string(source);
  if (auto_play_ && !source_.empty()) {
    Play();
  }
}

std::string_view MediaWidget::GetSource() const noexcept { return source_; }

void MediaWidget::SetFfplayPath(std::string_view path) {
  ffplay_path_ = std::string(path);
}

void MediaWidget::SetExtraArgs(std::string_view args) {
  extra_args_ = std::string(args);
}

void MediaWidget::SetAutoPlay(bool auto_play) noexcept {
  auto_play_ = auto_play;
}

void MediaWidget::Play() {
  if (source_.empty()) {
    LOG(WARNING) << "MediaWidget::Play: no source set";
    return;
  }
  if (state_ == MediaState::kPlaying) {
    return;
  }
  LaunchFfplay();
  state_ = MediaState::kPlaying;
}

void MediaWidget::Pause() {
  // ffplay has no IPC pause mechanism; stopping is the only option.
  Stop();
  state_ = MediaState::kPaused;
}

void MediaWidget::Stop() {
  TerminateFfplay();
  state_ = MediaState::kStopped;
}

MediaState MediaWidget::GetState() const noexcept { return state_; }

void MediaWidget::SetBackgroundColor(const Color& color) noexcept {
  background_color_ = color;
}

void MediaWidget::SetButtonColor(const Color& color) noexcept {
  button_color_ = color;
}

void MediaWidget::SetTextColor(const Color& color) noexcept {
  text_color_ = color;
}

void MediaWidget::LaunchFfplay() {
  if (process_handle_ != nullptr) {
    TerminateFfplay();
  }

#ifdef _WIN32
  auto* handle = new ProcessHandle();
  std::string cmd = "\"" + ffplay_path_ + "\" -autoexit";
  if (!extra_args_.empty()) {
    cmd += " " + extra_args_;
  }
  cmd += " \"" + source_ + "\"";
  STARTUPINFOA si{};
  si.cb = sizeof(si);
  if (!CreateProcessA(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0,
                      nullptr, nullptr, &si, &handle->pi)) {
    LOG(ERROR) << "MediaWidget: CreateProcess failed for ffplay: "
               << GetLastError();
    delete handle;
    return;
  }
  handle->valid = true;
  process_handle_ = handle;
  LOG(INFO) << "MediaWidget: launched ffplay (pid=" << handle->pi.dwProcessId
            << ")";
#else
  auto* handle = new ProcessHandle();
  pid_t pid = fork();
  if (pid < 0) {
    LOG(ERROR) << "MediaWidget: fork failed";
    delete handle;
    return;
  }
  if (pid == 0) {
    // Child process: exec ffplay with extra args if provided.
    // Build argument vector dynamically to support extra_args_.
    std::vector<std::string> arg_storage;
    arg_storage.emplace_back(ffplay_path_);
    arg_storage.emplace_back("-autoexit");
    if (!extra_args_.empty()) {
      // Split extra_args_ by spaces (simple tokenizer; does not handle
      // quoted args with spaces). For complex args, users should set
      // ffplay_path_ to a wrapper script.
      std::istringstream iss(extra_args_);
      std::string token;
      while (iss >> token) {
        arg_storage.emplace_back(std::move(token));
      }
    }
    arg_storage.emplace_back(source_);
    std::vector<char*> args;
    args.reserve(arg_storage.size() + 1);
    for (auto& s : arg_storage) {
      args.push_back(s.data());
    }
    args.push_back(nullptr);
    execvp(ffplay_path_.c_str(), args.data());
    // If exec returns, it failed.
    _exit(127);
  }
  handle->pid = pid;
  handle->valid = true;
  process_handle_ = handle;
  LOG(INFO) << "MediaWidget: launched ffplay (pid=" << pid << ")";
#endif
}

void MediaWidget::TerminateFfplay() {
  if (process_handle_ == nullptr) {
    return;
  }
  auto* handle = static_cast<ProcessHandle*>(process_handle_);
  if (handle->valid) {
#ifdef _WIN32
    TerminateProcess(handle->pi.hProcess, 0);
    WaitForSingleObject(handle->pi.hProcess, 1000);
    CloseHandle(handle->pi.hProcess);
    CloseHandle(handle->pi.hThread);
#else
    if (handle->pid > 0) {
      kill(handle->pid, SIGTERM);
      // Reap the child to avoid zombies.
      int status = 0;
      waitpid(handle->pid, &status, WNOHANG);
    }
#endif
    handle->valid = false;
  }
  delete handle;
  process_handle_ = nullptr;
}

bool MediaWidget::HitPlayButton(const Point& local_pos) const noexcept {
  const Rect& b = GetBounds();
  const float btn_size = std::min(b.width, b.height) * 0.25F;
  const float btn_x = (b.width - btn_size) * 0.5F;
  const float btn_y = (b.height - btn_size) * 0.5F;
  return local_pos.x >= btn_x && local_pos.x <= btn_x + btn_size &&
         local_pos.y >= btn_y && local_pos.y <= btn_y + btn_size;
}

}  // namespace neoflux
