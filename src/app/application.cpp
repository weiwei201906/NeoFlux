// =============================================================================
// NeoFlux - application.cpp
//
// Implementation of Application. Methods moved from header.
// =============================================================================

#include "neoflux/app/application.h"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "neoflux/core/types.h"
#include "neoflux/render/render_context.h"
#include "neoflux/render/render_layer.h"
#include "neoflux/widget/route_registry.h"
#include "neoflux/widget/widget.h"

DEFINE_int32(target_fps, 60, "Target frames per second for the event loop.");
DEFINE_bool(verbose_logging, false,
            "Enable verbose (VLOG) logging output.");
// logtostderr and log_dir are built-in glog flags; declare (not define) them.
DECLARE_bool(logtostderr);
DECLARE_string(log_dir);

namespace neoflux {

Application::Application() = default;

Application::~Application() { Stop(); }

bool Application::Init(int argc, char** argv, int window_width,
                       int window_height, std::string_view window_title,
                       void* platform_surface) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  // Configure glog logging destination. Default is file-based logging to
  // --log_dir (defaults to ./logs); --logtostderr redirects all output to
  // stderr (useful for debugging and CI environments).
  if (!FLAGS_logtostderr) {
    if (FLAGS_log_dir.empty()) {
      FLAGS_log_dir = "./logs";
    }
    std::error_code ec;
    std::filesystem::create_directories(FLAGS_log_dir, ec);
    if (ec) {
      // Fall back to stderr if the log directory cannot be created.
      FLAGS_logtostderr = true;
    }
  }

  // Disable log buffering so messages appear immediately.
  FLAGS_logbuflevel = -1;
  google::InitGoogleLogging(argv[0]);

  if (FLAGS_verbose_logging) {
    google::SetStderrLogging(google::GLOG_INFO);
    FLAGS_v = 1;
  }

  LOG(INFO) << "NeoFlux Application initializing";
  LOG(INFO) << "Window: " << window_width << "x" << window_height;
  if (!FLAGS_logtostderr) {
    LOG(INFO) << "Logging to directory: " << FLAGS_log_dir;
  }

  window_width_ = window_width;
  window_height_ = window_height;

  render_layer_ = std::make_unique<RenderLayer>();
  if (!render_layer_->Start(window_width, window_height, window_title,
                            platform_surface)) {
    LOG(ERROR) << "Failed to start render layer";
    return false;
  }

  event_loop_.SetTargetFps(FLAGS_target_fps);

  initialized_ = true;
  LOG(INFO) << "NeoFlux Application initialized successfully";
  return true;
}

void Application::Run() {  // NOLINT(readability-make-member-function-const)
  if (!initialized_) {
    LOG(ERROR) << "Application::Run called before Init()";
    return;
  }

  LOG(INFO) << "Starting application event loop";

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
    navigation_stack_.push_back(std::move(widget));
    LOG(INFO) << "Pushed route: " << route_name
              << " (stack depth: " << navigation_stack_.size() << ")";
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

int Application::GetWindowWidth() const noexcept { return window_width_; }

int Application::GetWindowHeight() const noexcept { return window_height_; }

void Application::OnFrame() {
  if (render_layer_ != nullptr) {
    render_layer_->PollEvents();
    if (render_layer_->ShouldClose()) {
      Stop();
      return;
    }
  }

  BuildDirtyWidgets();
  LayoutWidgetTree();
  PaintAndSubmit();
}

void Application::BuildDirtyWidgets() {
  BuildContext context(this);
  Widget* root = GetRootWidget();
  if (root != nullptr) {
    BuildWidgetRecursive(*root, context);
  }
}

void Application::BuildWidgetRecursive(Widget& widget, BuildContext& context) {
  if (widget.NeedsBuild()) {
    auto child = widget.Build(context);
    if (child != nullptr) {
      widget.ClearChildren();
      widget.AddChild(std::move(child));
    }
    widget.ClearNeedsBuild();
  }

  for (auto& child : widget.GetChildren()) {
    if (child != nullptr) {
      BuildWidgetRecursive(*child, context);
    }
  }
}

void Application::LayoutWidgetTree() {
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
    std::string children_info;
    for (const auto& child : root->GetChildren()) {
      if (child) {
        const auto& b = child->GetBounds();
        children_info += "[" + std::to_string(static_cast<int>(b.x)) + "," +
                         std::to_string(static_cast<int>(b.y)) + " " +
                         std::to_string(static_cast<int>(b.width)) + "x" +
                         std::to_string(static_cast<int>(b.height)) + "] ";
      }
    }
    VLOG(1) << "Layout: root " << root->GetBounds().width << "x"
            << root->GetBounds().height << ", children: " << children_info;
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

}  // namespace neoflux
