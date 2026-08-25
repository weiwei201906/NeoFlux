// =============================================================================
// NeoFlux - render_layer.cpp
//
// Implementation of RenderLayer. Owns the render thread, platform bridge
// (GLFW on desktop, MobileBridge on Android/iOS), and tgfx renderer.
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

#include "neoflux/native/desktop/glfw_bridge.h"
#include "neoflux/native/mobile/mobile_bridge.h"
#include "neoflux/render/render_command.h"
#include "neoflux/render/tgfx_renderer.h"

DEFINE_uint64(render_queue_capacity, 2048,
              "Capacity of the render command SPSC ring queue. "
              "One slot is reserved for full/empty distinction, so the "
              "maximum storable commands are (capacity - 1).");

DEFINE_string(render_backend, "vulkan",
              "Rendering backend to use. Options: 'vulkan' (default, "
              "currently falls back to GL), 'gl' (OpenGL), 'cpu' "
              "(software rasterizer, falls back to GL).");

namespace neoflux {

RenderLayer::RenderLayer() = default;

RenderLayer::~RenderLayer() { Stop(); }

bool RenderLayer::Start(int width, int height, std::string_view title,
                        std::string_view font_dir,
                        void* platform_surface) {
  if (running_.load()) {
    LOG(WARNING) << "RenderLayer already running";
    return false;
  }

  command_queue_.Init(FLAGS_render_queue_capacity);
  // Reset the promise so a fresh future can be obtained. A promise can only
  // produce one future; without this, a second Start() would throw.
  render_ready_ = std::promise<void>{};

  window_width_ = static_cast<std::uint16_t>(width);
  window_height_ = static_cast<std::uint16_t>(height);

  LOG(INFO) << "RenderLayer starting: " << width << "x" << height
            << " backend=" << FLAGS_render_backend;

  if (FLAGS_render_backend != "gl" && FLAGS_render_backend != "vulkan" &&
      FLAGS_render_backend != "cpu") {
    LOG(WARNING) << "Unknown render_backend '" << FLAGS_render_backend
                 << "', falling back to 'vulkan'";
  }
  if (FLAGS_render_backend == "vulkan") {
    LOG(WARNING) << "Vulkan backend is not yet implemented; falling back to GL";
  } else if (FLAGS_render_backend == "cpu") {
    LOG(WARNING) << "CPU software rasterizer not yet implemented; falling back to GL";
  }

  renderer_ = std::make_unique<TgfxRenderer>();

#ifdef NEOFLUX_PLATFORM_DESKTOP
  (void)platform_surface;  // Desktop creates its own GLFW window.
  // Desktop: create GLFW window + OpenGL context, tgfx renders into the
  // GLFW framebuffer. GLFW handles windowing, input, and buffer swap.
  auto glfw_bridge = std::make_unique<GlfwBridge>();
  if (!glfw_bridge->Init(width, height, title)) {
    LOG(ERROR) << "Failed to initialize GLFW bridge";
    return false;
  }

  // Temporarily make the GL context current on the main thread so that
  // renderer_->Init() can load GL function pointers via glfwGetProcAddress.
  glfw_bridge->MakeContextCurrent();

  if (!renderer_->Init(width, height, font_dir,
                       glfw_bridge->GetNativeHandle())) {
    LOG(ERROR) << "Failed to initialize tgfx renderer";
    GlfwBridge::ReleaseContext();
    glfw_bridge->Shutdown();
    return false;
  }

  // Release the context from the main thread; the render thread will
  // acquire it exclusively via MakeContextCurrent() in RenderLoop().
  GlfwBridge::ReleaseContext();

  platform_bridge_ = std::move(glfw_bridge);
#else
  // Mobile: create a platform bridge wrapping the native surface provided
  // by the OS (ANativeWindow on Android, CAMetalLayer/EAGLLayer on iOS).
  // tgfx renders directly into this surface; the platform manages display
  // refresh and surface lifecycle.
  platform_bridge_ = CreateMobileBridge(platform_surface, width, height);
  if (platform_bridge_ == nullptr) {
    LOG(ERROR) << "Failed to create mobile platform bridge";
    return false;
  }

  if (!renderer_->Init(width, height, font_dir, platform_surface)) {
    LOG(ERROR) << "Failed to initialize tgfx renderer (mobile)";
    platform_bridge_.reset();
    return false;
  }
#endif

  running_.store(true);
  // Obtain the future BEFORE launching the render thread. A default-constructed
  // std::future has no shared state and wait_for() would throw std::future_error.
  // Getting the future here also avoids a race where the render thread calls
  // set_value() before the main thread has called get_future() (which would
  // also throw).
  render_ready_future_ = render_ready_.get_future();
  render_thread_ = std::make_unique<std::thread>([this]() { RenderLoop(); });

  // Block until the render thread has made the context current and performed
  // a preliminary frame to initialise GL resources.
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
  platform_bridge_.reset();

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
  if (platform_bridge_ != nullptr) {
    return platform_bridge_->ShouldClose();
  }
  return should_close_.load();
}

void RenderLayer::PollEvents() {
  if (platform_bridge_ != nullptr) {
    platform_bridge_->PollEvents();
  }
}

PlatformBridge* RenderLayer::GetPlatformBridge() const noexcept {
  return platform_bridge_.get();
}

void RenderLayer::GetWindowSize(int& width, int& height) const noexcept {
#ifdef NEOFLUX_PLATFORM_DESKTOP
  if (platform_bridge_ != nullptr) {
    static_cast<GlfwBridge*>(platform_bridge_.get())
        ->GetWindowSize(width, height);
    return;
  }
#endif
  if (platform_bridge_ != nullptr) {
    width = platform_bridge_->GetWidth();
    height = platform_bridge_->GetHeight();
    return;
  }
  width = static_cast<int>(window_width_);
  height = static_cast<int>(window_height_);
}

void RenderLayer::RenderLoop() {
  LOG(INFO) << "Render thread started";

  // Make the rendering context current on the render thread. On desktop this
  // is the GLFW OpenGL context; on mobile it is the EAGL/Metal context owned
  // by the platform bridge.
  if (platform_bridge_ != nullptr) {
    platform_bridge_->MakeContextCurrent();
  }

  render_ready_.set_value();

  bool in_frame = false;
  std::uint64_t frames_rendered = 0;
  constexpr Color kClearColor{.r = 245, .g = 245, .b = 245, .a = 255};

  while (running_.load()) {
    {
      std::unique_lock<std::mutex> lock(frame_mutex_);
      frame_cv_.wait_for(lock, std::chrono::milliseconds(16), [this] {
        return frame_ready_ || !running_.load();
      });
      frame_ready_ = false;
    }

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
            if (platform_bridge_ != nullptr) {
              platform_bridge_->SwapBuffers();
            }
            ++frames_rendered;
            // Log every 64 frames using bitwise AND (2^6 - 1 = 63) instead
            // of modulo 60. Bitwise AND is 1 cycle vs ~20-40 for integer
            // division; 64 frames ~= 1.07s at 60 FPS, acceptable for logging.
            if ((frames_rendered & 63U) == 0U) {
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
