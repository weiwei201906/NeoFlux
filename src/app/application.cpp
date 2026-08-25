// =============================================================================
// NeoFlux - application.cpp
//
// Implementation of Application. Methods moved from header.
// =============================================================================

#include "neoflux/app/application.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "neoflux/core/types.h"
#include "neoflux/render/render_context.h"
#include "neoflux/render/render_layer.h"
#include "neoflux/render/glfw_bridge.h"
#include "neoflux/widget/route_registry.h"
#include "neoflux/widget/widget.h"

DEFINE_int32(target_fps, 60, "Target frames per second for the event loop.");
DEFINE_bool(verbose_logging, false,
            "Enable verbose (VLOG) logging output.");
// logtostderr and log_dir are glog built-in flag variables. glog defines them
// but (when built without gflags integration) does not register them with the
// gflags command-line parser. We DECLARE them here to access the variables,
// and pre-scan argv in Init() to handle --logtostderr / --log_dir=... manually
// before gflags parses the remaining flags.
DECLARE_bool(logtostderr);
DECLARE_string(log_dir);

namespace neoflux {

Application::Application() = default;

Application::~Application() { Stop(); }

bool Application::Init(int argc, char** argv, int window_width,
                       int window_height, std::string_view window_title,
                       void* platform_surface) {
  // glog (when built without gflags integration) defines FLAGS_logtostderr and
  // FLAGS_log_dir as variables but does not register them with the gflags
  // command-line parser. Pre-scan argv to handle these two flags manually,
  // then strip them from argv before gflags parses the rest.
  std::vector<char*> filtered_argv;
  filtered_argv.reserve(static_cast<std::size_t>(argc));
  for (int i = 0; i < argc; ++i) {
    std::string_view arg(argv[i]);
    if (arg == "--logtostderr" || arg == "-logtostderr") {
      FLAGS_logtostderr = true;
    } else if (arg == "--nologtostderr" || arg == "-nologtostderr") {
      FLAGS_logtostderr = false;
    } else if (arg.substr(0, 10) == "--log_dir=" || arg.substr(0, 9) == "-log_dir=") {
      const auto equal_pos = arg.find('=');
      if (equal_pos != std::string_view::npos) {
        FLAGS_log_dir = std::string(arg.substr(equal_pos + 1));
      }
    } else if ((arg == "--log_dir" || arg == "-log_dir") && i + 1 < argc) {
      FLAGS_log_dir = argv[++i];
    } else {
      filtered_argv.push_back(argv[i]);
    }
  }
  int filtered_argc =  // NOLINT(cppcoreguidelines-init-variables)
      static_cast<int>(filtered_argv.size());
  char** filtered_argv_ptr =  // NOLINT(cppcoreguidelines-init-variables)
      filtered_argv.data();

  // Initialize glog first so its built-in flags are accessible. Route initial
  // log messages to stderr temporarily until the log destination is configured,
  // then restore the user's selection (default: file-based logging).
  const bool user_wants_stderr = FLAGS_logtostderr;
  FLAGS_logtostderr = true;
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&filtered_argc, &filtered_argv_ptr, true);
  FLAGS_logtostderr = user_wants_stderr;

  // Configure glog logging destination. Default is file-based logging to
  // --log_dir (defaults to ./logs); --logtostderr redirects all output to
  // stderr (useful for debugging and CI environments). Log files are named
  // with a .log extension for easy identification.
  if (!FLAGS_logtostderr) {
    if (FLAGS_log_dir.empty()) {
      FLAGS_log_dir = "./logs";
    }
    std::error_code dir_error;
    std::filesystem::create_directories(FLAGS_log_dir, dir_error);
    if (dir_error) {
      // Fall back to stderr if the log directory cannot be created.
      FLAGS_logtostderr = true;
    } else {
      // Append .log extension so log files end with .log.
      google::SetLogFilenameExtension(".log");
    }
  }

  // Disable log buffering so messages appear immediately.
  FLAGS_logbuflevel = -1;

  if (FLAGS_verbose_logging) {
    google::SetStderrLogging(google::GLOG_INFO);
    FLAGS_v = 1;
  }

  LOG(INFO) << "NeoFlux Application initializing";
  LOG(INFO) << "Window: " << window_width << "x" << window_height;
  if (!FLAGS_logtostderr) {
    LOG(INFO) << "Logging to directory: " << FLAGS_log_dir;
  }

  window_width_ = static_cast<std::uint16_t>(window_width);
  window_height_ = static_cast<std::uint16_t>(window_height);

  render_layer_ = std::make_unique<RenderLayer>();
  if (!render_layer_->Start(window_width, window_height, window_title,
                            font_dir_, platform_surface)) {
    LOG(ERROR) << "Failed to start render layer";
    return false;
  }

  event_loop_.SetTargetFps(FLAGS_target_fps);

  // Wire up input events from the platform bridge to the widget tree.
  if (auto* bridge = render_layer_->GetPlatformBridge(); bridge != nullptr) {
    bridge->SetInputCallback([this](MouseButton button, InputAction action,
                                    const Point& pos) {
      DispatchPointerEvent(button, action, pos);
      MarkFrameDirty();
    });
    bridge->SetKeyCallback([this](const KeyEvent& event) {
      DispatchKeyEvent(event);
    });
    bridge->SetCharCallback([this](const CharEvent& event) {
      DispatchCharEvent(event);
    });
#ifdef NEOFLUX_PLATFORM_DESKTOP
    // Desktop-only callbacks: scroll and resize are GLFW-specific.
    // Mobile delivers these via the platform touch/lifecycle system.
    auto* glfw = static_cast<GlfwBridge*>(bridge);
    glfw->SetScrollCallback([this](double xoffset, double yoffset) {
      DispatchScrollEvent(xoffset, yoffset);
    });
    glfw->SetResizeCallback([this](int width, int height) {
      window_width_ = static_cast<std::uint16_t>(width);
      window_height_ = static_cast<std::uint16_t>(height);
      MarkFrameDirty();
    });
    glfw->SetMouseMoveCallback([this](const Point& pos) {
      DispatchPointerMove(pos);
    });
#endif
  }

  initialized_ = true;
  LOG(INFO) << "NeoFlux Application initialized successfully";
  return true;
}

void Application::Run() {
  if (!initialized_) {
    LOG(ERROR) << "Application::Run called before Init()";
    return;
  }

  LOG(INFO) << "Starting application event loop";

  // Pre-build the widget tree and perform an initial layout pass before
  // entering the event loop. This ensures Taitank nodes are fully
  // inserted and measured before the first paint.
  BuildDirtyWidgets();
  if (auto* root = GetRootWidget(); root != nullptr) {
    root->PerformLayout(static_cast<float>(window_width_),
                        static_cast<float>(window_height_));
  }

  frame_dirty_.store(true);
  event_loop_.Run([this]() { OnFrame(); });

  LOG(INFO) << "Application event loop ended";
}

void Application::Stop() {
  LOG(INFO) << "Application stopping";
  event_loop_.Stop();
  if (render_layer_ != nullptr) {
    render_layer_->Stop();
  }
}

void Application::PushRoute(std::string_view route_name) {
  BuildContext context(this);
  auto widget = RouteRegistry::Instance().BuildRoute(route_name, context);
  if (widget != nullptr) {
    widget->SetApplication(this);
    navigation_stack_.push_back(std::move(widget));
    LOG(INFO) << "Pushed route: " << route_name
              << " (stack depth: " << navigation_stack_.size() << ")";
    MarkFrameDirty();
  }
}

void Application::PopRoute() {
  if (navigation_stack_.size() <= 1) {
    LOG(WARNING) << "Cannot pop root route";
    return;
  }
  navigation_stack_.pop_back();
  LOG(INFO) << "Popped route (stack depth: " << navigation_stack_.size()
            << ")";
  MarkFrameDirty();
}

Widget* Application::GetRootWidget() const noexcept {
  if (navigation_stack_.empty()) {
    return nullptr;
  }
  return navigation_stack_.back().get();
}

RenderLayer& Application::GetRenderLayer() noexcept {
  return *render_layer_;
}

EventLoop& Application::GetEventLoop() noexcept { return event_loop_; }

int Application::GetWindowWidth() const noexcept {
  return static_cast<int>(window_width_);
}

int Application::GetWindowHeight() const noexcept {
  return static_cast<int>(window_height_);
}

void Application::MarkFrameDirty() noexcept {
  frame_dirty_.store(true);
  event_loop_.WakeUp();
}

void Application::SetFontDir(std::string_view dir) noexcept {
  font_dir_ = std::string(dir);
}

std::string_view Application::GetFontDir() const noexcept {
  return font_dir_;
}

void Application::OnFrame() {
  if (render_layer_ != nullptr) {
    render_layer_->PollEvents();
    if (render_layer_->ShouldClose()) {
      Stop();
      return;
    }
  }

  // Always process dirty widgets: MarkNeedsBuild may be called without
  // going through MarkFrameDirty (e.g. from State::SetState).
  const bool rebuilt = BuildDirtyWidgets();
  const bool dirty = frame_dirty_.exchange(false);
  // Skip layout/paint when nothing changed: no explicit dirty flag and
  // no widget was rebuilt.
  if (!dirty && !rebuilt) {
    return;
  }

  LayoutWidgetTree();
  InvalidateHitCache();
  PaintAndSubmit();
}

bool Application::BuildDirtyWidgets() {
  BuildContext context(this);
  Widget* root = GetRootWidget();
  if (root == nullptr) {
    return false;
  }
  return BuildWidgetRecursive(*root, context);
}

bool Application::BuildWidgetRecursive(Widget& widget, BuildContext& context) {  bool rebuilt = false;
  if (widget.NeedsBuild()) {
    auto child = widget.Build(context);
    if (child != nullptr) {
      widget.ClearChildren();
      widget.AddChild(std::move(child));
    }
    widget.ClearNeedsBuild();
    rebuilt = true;
  }

  for (auto& child : widget.GetChildren()) {
    if (child != nullptr) {
      rebuilt |= BuildWidgetRecursive(*child, context);
    }
  }
  return rebuilt;
}

void Application::LayoutWidgetTree() const {
  Widget* root = GetRootWidget();
  if (root == nullptr) {
    return;
  }
  // Taitank flex layout: the root fills the entire window. PerformLayout
  // calls taitank::DoLayout on the root node and recursively copies
  // computed bounds back into the widget tree.
  root->PerformLayout(static_cast<float>(window_width_),
                      static_cast<float>(window_height_));
  if (VLOG_IS_ON(1)) {
    std::function<void(const Widget&, int)> dump = [&](const Widget& w,
                                                        int depth) {
      const auto& b = w.GetBounds();
      std::string indent(static_cast<std::size_t>(depth) * 2, ' ');
      VLOG(1) << indent << w.GetWidgetName() << " [" << static_cast<int>(b.x)
              << "," << static_cast<int>(b.y) << " "
              << static_cast<int>(b.width) << "x"
              << static_cast<int>(b.height) << "]";
      for (const auto& child : w.GetChildren()) {
        if (child != nullptr) {
          dump(*child, depth + 1);
        }
      }
    };
    dump(*root, 0);
  }
}

void Application::PaintAndSubmit() {
  Widget* root = GetRootWidget();
  if (root == nullptr || render_layer_ == nullptr) {
    return;
  }

  render_context_.Clear();

  // Frame boundary: kBeginFrame tells the render thread to start a new frame.
  render_context_.AppendCommand(RenderCommand::MakeBeginFrame());

  render_context_.Save();
  PaintWidgetRecursive(*root, render_context_);
  render_context_.Restore();

  // Frame boundary: kEndFrame tells the render thread to submit and swap.
  render_context_.AppendCommand(RenderCommand::MakeEndFrame());

  const auto& commands = render_context_.GetCommands();
  if (!commands.empty()) {
    const auto submitted = render_layer_->Submit(commands.data(), commands.size());
    VLOG(1) << "Submitted " << submitted << "/" << commands.size() << " render commands";
  }
}

void Application::PaintWidgetRecursive(Widget& widget,
                                       RenderContext& context) {
  widget.Paint(context);
}

void Application::DispatchPointerEvent(
    MouseButton button, InputAction action, const Point& pos) {
  if (button != MouseButton::kLeft) {
    return;
  }
  Widget* root = GetRootWidget();
  if (root == nullptr) {
    return;
  }

  // Scale cursor coordinates from actual window size to layout size.
  // On Windows with DPI virtualisation, glfwGetCursorPos returns logical
  // coordinates that may differ from the requested layout size.
  Point scaled_pos = pos;
  if (render_layer_ != nullptr) {
    int actual_w = 0;
    int actual_h = 0;
    render_layer_->GetWindowSize(actual_w, actual_h);
    if (actual_w > 0 && actual_h > 0) {
      scaled_pos.x =
          pos.x * static_cast<float>(window_width_) / static_cast<float>(actual_w);
      scaled_pos.y = pos.y * static_cast<float>(window_height_) /
                     static_cast<float>(actual_h);
    }
  }

  if (action == InputAction::kPress) {
    std::shared_ptr<Widget> hit = root->HitTest(scaled_pos);
    if (hit != nullptr) {
      const Point global_pos = hit->GetGlobalPosition();
      const Point paint_offset = hit->GetPaintOffset();
      const Point local{.x = scaled_pos.x - global_pos.x - paint_offset.x,
                        .y = scaled_pos.y - global_pos.y - paint_offset.y,};
      if (hit->OnPointerDown(local)) {
        // Store a weak_ptr so a widget-tree rebuild between press and release
        // does not leave a dangling pointer.
        pressed_widget_ = hit->weak_from_this();
        VLOG(1) << "Pointer press consumed by " << hit->GetWidgetName();
      }
      // Set keyboard focus if the clicked widget is focusable; otherwise
      // clear focus so typing does not go to a stale widget.
      SetFocus(hit->IsFocusable() ? hit : nullptr);
    }
  } else if (action == InputAction::kRelease) {
    auto pressed = pressed_widget_.lock();
    VLOG(1) << "Pointer release at (" << scaled_pos.x << "," << scaled_pos.y
            << ") pressed_widget valid: " << (pressed != nullptr);
    if (pressed != nullptr) {
      const Point global_pos = pressed->GetGlobalPosition();
      const Point paint_offset = pressed->GetPaintOffset();
      const Point local{.x = scaled_pos.x - global_pos.x - paint_offset.x,
                        .y = scaled_pos.y - global_pos.y - paint_offset.y,};
      pressed->OnPointerUp(local);
    }
    pressed_widget_.reset();
  }
  // Press/release changes visual state (button press, drag start/end), so
  // always mark the frame dirty for a repaint.
  MarkFrameDirty();
}

void Application::InvalidateHitCache() noexcept { hit_cache_valid_ = false; }

void Application::DispatchPointerMove(const Point& raw_pos) {
  Widget* root = GetRootWidget();
  if (root == nullptr || render_layer_ == nullptr) {
    return;
  }
  // Scale cursor coordinates from actual window size to layout size.
  Point pos = raw_pos;
  int actual_w = 0;
  int actual_h = 0;
  render_layer_->GetWindowSize(actual_w, actual_h);
  if (actual_w > 0 && actual_h > 0) {
    pos.x = raw_pos.x * static_cast<float>(window_width_) /
            static_cast<float>(actual_w);
    pos.y = raw_pos.y * static_cast<float>(window_height_) /
            static_cast<float>(actual_h);
  }
  // When a pointer is currently pressed (dragging), deliver move events
  // directly to the pressed widget regardless of hit-test results. This
  // ensures drag operations continue even when the cursor moves outside
  // the widget's layout bounds (e.g. Draggable with paint-time offset).
  std::shared_ptr<Widget> hit = pressed_widget_.lock();
  if (hit == nullptr) {
    // Use cached hit-test result when valid; otherwise recompute and cache.
    // Pointer-move fires at high frequency, so avoiding a full tree traversal
    // per event is critical for large widget trees.
    if (hit_cache_valid_) {
      hit = hit_cache_.lock();
    }
    if (hit == nullptr) {
      hit = root->HitTest(pos);
      hit_cache_ = hit;
      hit_cache_valid_ = true;
    }
  } else {
    // Pressed widget takes precedence; invalidate hover cache since the
    // cursor may be over a different widget during drag.
    hit_cache_valid_ = false;
  }
  // Detect enter/exit transitions only when not dragging (no pressed widget).
  if (pressed_widget_.expired()) {
    auto previous = hovered_widget_.lock();
    if (hit.get() != previous.get()) {
      if (previous != nullptr) {
        previous->OnPointerExit();
      }
      if (hit != nullptr) {
        hit->OnPointerEnter();
      }
      hovered_widget_ = hit;
      // Update the mouse cursor shape based on the hovered widget.
      // TextField requests an I-beam cursor; clickable widgets may request
      // a hand cursor; everything else uses the default arrow.
      if (render_layer_ != nullptr) {
        auto* bridge = render_layer_->GetPlatformBridge();
        if (bridge != nullptr) {
          if (hit != nullptr && hit->HasTextInputCursor()) {
            bridge->SetCursor(PlatformBridge::CursorType::kIBeam);
          } else {
            bridge->SetCursor(PlatformBridge::CursorType::kArrow);
          }
        }
      }
    }
  }
  // Deliver move event to the widget under the cursor.
  if (hit != nullptr) {
    const Point global_pos = hit->GetGlobalPosition();
    const Point paint_offset = hit->GetPaintOffset();
    const Point local{.x = pos.x - global_pos.x - paint_offset.x,
                      .y = pos.y - global_pos.y - paint_offset.y,};
    hit->OnPointerMove(local);
  }
  // Move may change visual state (drag offset, hover), so mark the frame
  // dirty for a repaint. This replaces the old MarkNeedsBuild() approach
  // that caused full widget-tree rebuilds during high-frequency drags.
  MarkFrameDirty();
}

void Application::DispatchScrollEvent(
    double xoffset, double yoffset) {
  Widget* root = GetRootWidget();
  if (root == nullptr || render_layer_ == nullptr) {
    return;
  }
#ifdef NEOFLUX_PLATFORM_DESKTOP
  // Scroll (mouse wheel) is desktop-only; mobile uses touch dragging.
  auto* bridge = static_cast<GlfwBridge*>(render_layer_->GetPlatformBridge());
  if (bridge == nullptr) {
    return;
  }
  const Point cursor_pos = bridge->GetCursorPos();
  // Scale cursor coordinates from actual window size to layout size.
  Point pos = cursor_pos;
  int actual_w = 0;
  int actual_h = 0;
  render_layer_->GetWindowSize(actual_w, actual_h);
  if (actual_w > 0 && actual_h > 0) {
    pos.x = cursor_pos.x * static_cast<float>(window_width_) /
            static_cast<float>(actual_w);
    pos.y = cursor_pos.y * static_cast<float>(window_height_) /
            static_cast<float>(actual_h);
  }
  std::shared_ptr<Widget> hit = root->HitTest(pos);
  // Scroll events bubble up: try the hit widget first, then each ancestor
  // until one consumes the event.
  while (hit != nullptr) {
    const Point global_pos = hit->GetGlobalPosition();
    const Point local{.x = pos.x - global_pos.x, .y = pos.y - global_pos.y};
    if (hit->OnPointerScroll(local, xoffset, yoffset)) {
      MarkFrameDirty();
      return;
    }
    Widget* parent = hit->GetParent();
    hit = (parent != nullptr) ? parent->shared_from_this() : nullptr;
  }
#endif
}

void Application::DispatchKeyEvent(const KeyEvent& event) {
  auto focused = focused_widget_.lock();
  if (focused != nullptr) {
    if (focused->OnKeyEvent(event)) {
      MarkFrameDirty();
    }
  }
}

void Application::DispatchCharEvent(const CharEvent& event) {
  auto focused = focused_widget_.lock();
  VLOG(1) << "DispatchCharEvent codepoint=" << event.codepoint
          << " focused=" << (focused ? focused->GetWidgetName() : "null");
  if (focused != nullptr) {
    if (focused->OnCharEvent(event)) {
      MarkFrameDirty();
    }
  }
}

void Application::SetFocus(std::shared_ptr<Widget> widget) {
  auto previous = focused_widget_.lock();
  if (previous.get() == widget.get()) {
    return;
  }
  if (previous != nullptr) {
    previous->OnBlur();
  }
  if (widget != nullptr && widget->IsFocusable()) {
    widget->OnFocus();
    focused_widget_ = widget;
  } else {
    focused_widget_.reset();
  }
  MarkFrameDirty();
}

std::shared_ptr<Widget> Application::GetFocusedWidget() const {
  return focused_widget_.lock();
}

}  // namespace neoflux
