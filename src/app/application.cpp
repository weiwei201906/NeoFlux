// =============================================================================
// NeoFlux - application.cpp
//
// Implementation of Application. Methods moved from header.
// =============================================================================

#include "neoflux/app/application.h"

#include <memory>
#include <string>
#include <utility>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "neoflux/widget/route_registry.h"

DEFINE_int32(target_fps, 60, "Target frames per second");
DEFINE_bool(verbose_logging, false, "Enable verbose logging");

namespace neoflux {

Application::Application() = default;

Application::~Application() { Stop(); }

bool Application::Init(int argc, char** argv, int window_width,
                       int window_height, std::string_view window_title,
                       void* platform_surface) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  google::InitGoogleLogging(argv[0]);
  if (FLAGS_verbose_logging) {
    google::SetStderrLogging(google::GLOG_INFO);
  }

  LOG(INFO) << "NeoFlux Application initializing";
  LOG(INFO) << "Window: " << window_width << "x" << window_height;

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

  LayoutConstraints constraints;
  constraints.min_width = 0.0F;
  constraints.max_width = static_cast<float>(window_width_);
  constraints.min_height = 0.0F;
  constraints.max_height = static_cast<float>(window_height_);

  const Size root_size = LayoutWidgetRecursive(*root, constraints);
  root->SetBounds({.x = 0.0F, .y = 0.0F, .width = root_size.width, .height = root_size.height});
}

Size Application::LayoutWidgetRecursive(
    Widget& widget, const LayoutConstraints& constraints) {
  return widget.Layout(constraints);
}

void Application::PaintAndSubmit() {
  Widget* root = GetRootWidget();
  if (root == nullptr || render_layer_ == nullptr) {
    return;
  }

  render_context_.Clear();

  render_context_.Save();
  PaintWidgetRecursive(*root, render_context_);
  render_context_.Restore();

  const auto& commands = render_context_.GetCommands();
  if (!commands.empty()) {
    render_layer_->Submit(commands.data(), commands.size());
  }
}

void Application::PaintWidgetRecursive(Widget& widget,
                                       RenderContext& context) {
  widget.Paint(context);
}

}  // namespace neoflux
