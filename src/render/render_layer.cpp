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

DEFINE_string(render_backend, "gl",
              "Rendering backend to use. Options: 'gl' (OpenGL ES 3.0 / "
              "OpenGL 3.3 core, default), 'vulkan' (Vulkan — currently a "
              "compile-time stub, falls back to GL with a warning).");

namespace neoflux {

RenderLayer::RenderLayer()  // NOLINT(cppcoreguidelines-pro-type-member-init, modernize-use-equals-default)
    : command_queue_(FLAGS_render_queue_capacity),
      running_(false),
      should_close_(false),
      render_thread_(nullptr),
      renderer_(nullptr),
      glfw_bridge_(nullptr),
      render_ready_future_(render_ready_.get_future()) {}

RenderLayer::~RenderLayer() { Stop(); }

bool RenderLayer::Start(int width, int height, std::string_view title,
                        void* /*platform_surface*/) {
  if (running_.load()) {
    LOG(WARNING) << "RenderLayer already running";
    return false;
  }

  window_width_ = width;
  window_height_ = height;

  LOG(INFO) << "RenderLayer starting: " << width << "x" << height
            << " backend=" << FLAGS_render_backend;

  if (FLAGS_render_backend != "gl" && FLAGS_render_backend != "vulkan") {
    LOG(WARNING) << "Unknown render_backend '" << FLAGS_render_backend
                 << "', falling back to 'gl'";
  }
  if (FLAGS_render_backend == "vulkan") {
    LOG(WARNING) << "Vulkan backend is not yet implemented; falling back to GL";
  }

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

  // Temporarily make the GL context current on the main thread so that
  // renderer_->Init() can load GL function pointers via glfwGetProcAddress.
  // On Windows, wglGetProcAddress requires a current context; without it
  // all function pointers resolve to NULL and the first frame renders
  // nothing (window appears black until an input event triggers a re-render
  // after the render thread has made the context current).
  glfw_bridge_->MakeContextCurrent();

  if (!renderer_->Init(width, height, glfw_bridge_->GetNativeHandle())) {
    LOG(ERROR) << "Failed to initialize tgfx renderer";
    glfw_bridge_->ReleaseContext();
    glfw_bridge_->Shutdown();
    glfw_bridge_.reset();
    return false;
  }

  // Release the context from the main thread; the render thread will
  // acquire it exclusively via MakeContextCurrent() in RenderLoop().
  glfw_bridge_->ReleaseContext();

  // Note: renderer_->Init() already stored the logical window size for
  // u_resolution (shader layout coordinates). The actual framebuffer size
  // (which may differ due to DPI scaling) is queried each frame in
  // TgfxRenderer::BeginFrame() and used only for glViewport. Do NOT call
  // Resize() here with the framebuffer size — that would corrupt u_resolution
  // and make layout coordinates mismatch the shader.
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

  // Block until the render thread has made the GL context current and
  // performed a preliminary frame to initialise GL resources (shaders,
  // FBOs, font textures). Without this, the first real frame submitted by
  // the application can race GL initialisation and render partially or
  // not at all until an input event triggers a second frame.
  if (render_ready_future_.wait_for(std::chrono::seconds(5)) ==
      std::future_status::timeout) {
    LOG(WARNING) << "Render thread did not become ready within 5s; "
                    "continuing anyway (first frame may be incomplete)";
  }

  LOG(INFO) << "RenderLayer started successfully";
  return true;
}

void RenderLayer::Stop() {
  if (!running_.exchange(false)) {
    return;
  }

  LOG(INFO) << "RenderLayer stopping";

  // Wake the render thread so it can exit the wait loop.
  {
    std::lock_guard<std::mutex> lock(frame_mutex_);
    frame_ready_ = true;
  }
  frame_cv_.notify_one();

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

  // Wake the render thread: a new frame (or partial frame) is available.
  if (submitted > 0) {
    {
      std::lock_guard<std::mutex> lock(frame_mutex_);
      frame_ready_ = true;
    }
    frame_cv_.notify_one();
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

GlfwBridge* RenderLayer::GetGlfwBridge() const noexcept {
  return glfw_bridge_.get();
}

void RenderLayer::GetWindowSize(int& width, int& height) const noexcept {
#ifdef NEOFLUX_PLATFORM_DESKTOP
  if (glfw_bridge_ != nullptr) {
    glfw_bridge_->GetWindowSize(width, height);
    return;
  }
#endif
  width = window_width_;
  height = window_height_;
}

void RenderLayer::RenderLoop() {
  LOG(INFO) << "Render thread started";

#ifdef NEOFLUX_PLATFORM_DESKTOP
  // Make the OpenGL context current on the render thread. The context was
  // created in GlfwBridge::Init but not bound, so this thread owns it
  // exclusively for all rendering and buffer swap operations.
  if (glfw_bridge_ != nullptr) {
    glfw_bridge_->MakeContextCurrent();
  }
#endif

  // Signal readiness immediately: the GL context is current and the renderer
  // has been initialised in Start(). The first real frame from the
  // application will trigger BeginFrame() which lazily initialises GL
  // resources (shaders, FBOs, font textures) on this thread.
  render_ready_.set_value();

  // Frame state machine: only render commands between kBeginFrame and
  // kEndFrame are drawn. This eliminates flicker caused by rendering
  // partial frames while the main thread is still submitting commands.
  bool in_frame = false;
  std::uint64_t frames_rendered = 0;
  constexpr Color kClearColor{.r = 245, .g = 245, .b = 245, .a = 255};

  while (running_.load()) {
    // Wait for a frame to be submitted (or stop signal). Uses a condition
    // variable instead of busy-polling to avoid wasting CPU cycles.
    {
      std::unique_lock<std::mutex> lock(frame_mutex_);
      frame_cv_.wait_for(lock, std::chrono::milliseconds(16), [this] {
        return frame_ready_ || !running_.load();
      });
      frame_ready_ = false;
    }

    // Drain all available commands for this frame.
    RenderCommand cmd;
    while (running_.load() && command_queue_.TryPop(cmd)) {
      switch (cmd.type) {
        case RenderCommandType::kBeginFrame:
          if (renderer_ != nullptr) {
            renderer_->BeginFrame(kClearColor);
          }
          in_frame = true;
          break;
        case RenderCommandType::kEndFrame:
          if (in_frame && renderer_ != nullptr) {
            renderer_->EndFrame();
#ifdef NEOFLUX_PLATFORM_DESKTOP
            if (glfw_bridge_ != nullptr) {
              glfw_bridge_->SwapBuffers();
            }
#endif
            ++frames_rendered;
            if (frames_rendered % 60 == 0) {
              LOG(INFO) << "Rendered " << frames_rendered << " frames";
            }
          }
          in_frame = false;
          break;
        default:
          if (in_frame && renderer_ != nullptr) {
            renderer_->Execute(cmd);
          }
          break;
      }
    }
  }

  LOG(INFO) << "Render thread exiting";
}

}  // namespace neoflux
