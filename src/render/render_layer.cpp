// =============================================================================
// NeoFlux - render_layer.cpp
//
// Implementation of RenderLayer. Methods moved from header.
// =============================================================================

#include "neoflux/render/render_layer.h"

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "neoflux/render/glfw_bridge.h"
#include "neoflux/render/render_command.h"
#include "neoflux/render/tgfx_renderer.h"

DEFINE_uint64(render_queue_capacity, 2048,
              "Capacity of the render command SPSC ring queue. "
              "One slot is reserved for full/empty distinction, so the "
              "maximum storable commands are (capacity - 1).");

namespace neoflux {

RenderLayer::RenderLayer()  // NOLINT(cppcoreguidelines-pro-type-member-init)
    : command_queue_(FLAGS_render_queue_capacity),
      running_(false),
      should_close_(false),
      render_thread_(nullptr),
      renderer_(nullptr),
      glfw_bridge_(nullptr) {}

RenderLayer::~RenderLayer() { Stop(); }

bool RenderLayer::Start(int width, int height, std::string_view title,
                        void* /*platform_surface*/) {
  if (running_.load()) {
    LOG(WARNING) << "RenderLayer already running";
    return false;
  }

  window_width_ = width;
  window_height_ = height;

  LOG(INFO) << "RenderLayer starting: " << width << "x" << height;

  renderer_ = std::make_unique<TgfxRenderer>();

#ifdef NEOFLUX_PLATFORM_DESKTOP
  // Desktop: create GLFW window + OpenGL context, tgfx renders into the
  // GLFW framebuffer. GLFW handles windowing, input, and buffer swap.
  glfw_bridge_ = std::make_unique<GlfwBridge>();
  if (!glfw_bridge_->Init(width, height, title)) {
    LOG(ERROR) << "Failed to initialize GLFW bridge";
    glfw_bridge_.reset();
    return false;
  }

  if (!renderer_->Init(width, height, glfw_bridge_->GetNativeHandle())) {
    LOG(ERROR) << "Failed to initialize tgfx renderer";
    glfw_bridge_->Shutdown();
    glfw_bridge_.reset();
    return false;
  }
#else
  // Mobile: tgfx renders directly into the platform surface provided by
  // the OS (ANativeWindow / CAMetalLayer). No windowing bridge is needed;
  // the platform manages surface lifecycle and display refresh.
  if (platform_surface == nullptr) {
    LOG(ERROR) << "Mobile platform surface is required for tgfx initialization";
    return false;
  }

  if (!renderer_->Init(width, height, platform_surface)) {
    LOG(ERROR) << "Failed to initialize tgfx renderer (mobile)";
    return false;
  }
#endif

  running_.store(true);
  render_thread_ = std::make_unique<std::thread>([this]() { RenderLoop(); });

  LOG(INFO) << "RenderLayer started successfully";
  return true;
}

void RenderLayer::Stop() {
  if (!running_.exchange(false)) {
    return;
  }

  LOG(INFO) << "RenderLayer stopping";

  if (render_thread_ != nullptr && render_thread_->joinable()) {
    render_thread_->join();
  }
  render_thread_.reset();

  renderer_.reset();

  if (glfw_bridge_ != nullptr) {
    glfw_bridge_->Shutdown();
    glfw_bridge_.reset();
  }

  LOG(INFO) << "RenderLayer stopped";
}

std::size_t RenderLayer::Submit(const RenderCommand* commands,
                                std::size_t count) {
  if (commands == nullptr || count == 0) {
    return 0;
  }

  std::size_t submitted = 0;
  for (std::size_t i = 0; i < count; ++i) {
    if (!command_queue_.TryPush(commands[i])) {
      LOG_FIRST_N(WARNING, 10)
          << "Render command queue full, dropped " << (count - i)
          << " commands";
      break;
    }
    ++submitted;
  }
  return submitted;
}

bool RenderLayer::IsRunning() const noexcept { return running_.load(); }

bool RenderLayer::ShouldClose() const {
#ifdef NEOFLUX_PLATFORM_DESKTOP
  if (glfw_bridge_ != nullptr) {
    return glfw_bridge_->ShouldClose();
  }
#endif
  return should_close_.load();
}

void RenderLayer::PollEvents() {
#ifdef NEOFLUX_PLATFORM_DESKTOP
  if (glfw_bridge_ != nullptr) {
    glfw_bridge_->PollEvents();
  }
#endif
}

void RenderLayer::RenderLoop() {
  LOG(INFO) << "Render thread started";

  while (running_.load()) {
    if (renderer_ != nullptr) {
      renderer_->BeginFrame({245, 245, 245, 255});
      ProcessPendingCommands();
      renderer_->EndFrame();
    }

#ifdef NEOFLUX_PLATFORM_DESKTOP
    if (glfw_bridge_ != nullptr) {
      glfw_bridge_->SwapBuffers();
    }
#endif

    if (command_queue_.Empty()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  LOG(INFO) << "Render thread exiting";
}

void RenderLayer::ExecuteCommand(const RenderCommand& command) {
  if (renderer_ == nullptr) {
    return;
  }

  // NOLINTBEGIN(bugprone-branch-clone)
  switch (command.type) {
    case RenderCommandType::kBeginFrame:
      renderer_->BeginFrame({245, 245, 245, 255});
      break;
    case RenderCommandType::kEndFrame:
      renderer_->EndFrame();
      break;
    default:
      renderer_->Execute(command);
      break;
  }
  // NOLINTEND(bugprone-branch-clone)
}

void RenderLayer::ProcessPendingCommands() {
  RenderCommand cmd;
  while (command_queue_.TryPop(cmd)) {
    ExecuteCommand(cmd);
  }
}

}  // namespace neoflux
