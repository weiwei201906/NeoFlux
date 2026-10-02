// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - render_layer.cpp
//
// Implementation of RenderLayer. Methods moved from header.
// =============================================================================

#include "neoflux/renderers/render_layer.h"

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "neoflux/renderers/glfw_bridge.h"
#include "neoflux/core/config.h"
#include "neoflux/renderers/render_command.h"
#include "neoflux/renderers/tgfx_renderer.h"

DEFINE_uint64(render_queue_capacity, neoflux::config::kDefaultRenderQueueCapacity,
              "Capacity of the render command SPSC ring queue. "
              "One slot is reserved for full/empty distinction, so the "
              "maximum storable commands are (capacity - 1).");

DEFINE_int32(render_queue_drop_log_max, 10,
             "Maximum number of times a 'render command queue full, dropped "
             "commands' warning is emitted per process. After this many "
             "occurrences, subsequent drops are counted silently.");

namespace neoflux {

RenderLayer::RenderLayer()  // NOLINT(cppcoreguidelines-pro-type-member-init, modernize-use-equals-default)
    : command_queue_(FLAGS_render_queue_capacity),
      running_(false),
      should_close_(false),
      render_thread_(nullptr),
      render_ready_future_(render_ready_.get_future()),
      renderer_(nullptr),
      glfw_bridge_(nullptr) {}

RenderLayer::~RenderLayer() {  // NOLINT(bugprone-exception-escape): all Stop() exceptions are caught below
  // Destructors must not throw; Stop() joins threads and waits on futures,
  // which can theoretically throw. Swallow exceptions during teardown.
  try {
    Stop();
  } catch (const std::exception& e) {
    LOG(WARNING) << "Exception during RenderLayer teardown: " << e.what();
  } catch (...) {
    LOG(WARNING) << "Unknown exception during RenderLayer teardown";
  }
}

bool RenderLayer::Start(int width, int height, std::string_view title,
                        void* /*platform_surface*/) {
  if (running_.load()) {
    LOG(WARNING) << "RenderLayer already running";
    return false;
  }

  window_width_ = width;
  window_height_ = height;

#if defined(NEOFLUX_BACKEND_gl)
  LOG(INFO) << "RenderLayer starting: " << width << "x" << height
            << " backend=gl (tgfx OpenGL)";
  LOG(INFO) << "Using tgfx OpenGL backend";
#elif defined(NEOFLUX_BACKEND_vulkan)
  LOG(INFO) << "RenderLayer starting: " << width << "x" << height
            << " backend=vulkan (tgfx Vulkan)";
  LOG(INFO) << "Using tgfx Vulkan backend";
#elif defined(NEOFLUX_BACKEND_d3d12)
  LOG(INFO) << "RenderLayer starting: " << width << "x" << height
            << " backend=d3d12 (tgfx D3D12)";
  LOG(INFO) << "Using tgfx D3D12 backend";
#elif defined(NEOFLUX_BACKEND_metal)
  LOG(INFO) << "RenderLayer starting: " << width << "x" << height
            << " backend=metal (tgfx Metal)";
  LOG(INFO) << "Using tgfx Metal backend";
#else
  LOG(INFO) << "RenderLayer starting: " << width << "x" << height;
#endif

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
    GlfwBridge::ReleaseContext();
    glfw_bridge_->Shutdown();
    glfw_bridge_.reset();
    return false;
  }

  // Release the context from the main thread; the render thread will
  // acquire it exclusively via MakeContextCurrent() in RenderLoop().
  GlfwBridge::ReleaseContext();

  // Note: renderer_->Init() already stored the logical window size for
  // u_resolution (shader layout coordinates). The actual framebuffer size
  // (which may differ due to DPI scaling) is queried each frame in
  // TgfxRenderer::BeginFrame() and used only for glViewport. Do NOT call
  // Resize() here with the framebuffer size -- that would corrupt u_resolution
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

  // Wake the render thread so it can exit the wait loop. The render thread
  // releases the WGL context on itself before returning (see RenderLoop), so by
  // the time join() returns it is safe for the App thread to destroy the window.
  Wake();

  if (render_thread_ != nullptr && render_thread_->joinable()) {
    render_thread_->join();
  }
  render_thread_.reset();

  // Intentionally abandon the renderer rather than destroy it here. tgfx's
  // GLDevice teardown requires the WGL context to be current AND a full
  // releaseAll() protocol that this app does not implement; destroying it on the
  // App thread deadlocks (it issues GL calls / blocks on a context lock owned by
  // the render thread), while destroying it on the render thread trips tgfx's
  // debug assertions. The process is about to exit, so we leak the small host-side
  // object and let the OS reclaim all GPU resources when the window/GL context is
  // destroyed below.
  renderer_.release();

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
      // Rate-limit the drop warning: emit up to FLAGS_render_queue_drop_log_max
      // times per process. LOG_FIRST_N requires a compile-time constant, so we
      // implement the cap with a relaxed atomic counter keyed off the runtime gflag.
      static std::atomic<int> drop_log_count{0};
      if (drop_log_count.fetch_add(1, std::memory_order_relaxed) <
          FLAGS_render_queue_drop_log_max) {
        LOG(WARNING) << "Render command queue full, dropped " << (count - i)
                     << " commands";
      }
      break;
    }
    ++submitted;
  }

  // Wake the render thread: a new frame (or partial frame) is available.
  if (submitted > 0) {
    Wake();
  }
  return submitted;
}

void RenderLayer::Wake() {
  {
    std::scoped_lock lock(frame_mutex_);
    frame_ready_ = true;
  }
  frame_cv_.notify_one();
}

void RenderLayer::SetRenderPump(std::function<void()> pump) {
  {
    std::scoped_lock lock(frame_mutex_);
    render_pump_ = std::move(pump);
  }
  // If a pump was just installed, wake the loop so it takes effect promptly.
  frame_cv_.notify_one();
}

void RenderLayer::RunOnRenderThread(std::function<void()> task) {
  if (task == nullptr) {
    return;
  }
  if (!running_.load()) {
    // No render thread is alive to service the task. The GL context/window has
    // already been torn down, so any GL names the task would free are reclaimed
    // by the driver. Drop the task rather than run it on this thread (which has
    // no current GL context and would itself crash).
    LOG(WARNING) << "RunOnRenderThread: render thread not running, dropping task";
    return;
  }
  // Signal completion once the render thread has executed the wrapped task, so
  // the caller (App thread) blocks until GL teardown has actually happened.
  auto done = std::make_shared<std::promise<void>>();
  std::future<void> fut = done->get_future();
  {
    std::scoped_lock lock(frame_mutex_);
    render_tasks_.push_back([task = std::move(task), done]() mutable {
      task();
      done->set_value();
    });
    frame_ready_ = true;
  }
  frame_cv_.notify_one();
  fut.wait();
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
    // Wait for work: a submitted frame, an external frame signal (e.g. mpv,
    // woken via Wake()), or the stop signal. There is intentionally NO fixed
    // timeout: the render thread sleeps until notified, which eliminates the
    // previous fixed 16ms idle poll.
    std::function<void()> pump;
    {
      std::unique_lock<std::mutex> lock(frame_mutex_);
      frame_cv_.wait(lock, [this] {
        return frame_ready_ || !running_.load();
      });
      frame_ready_ = false;
      pump = render_pump_;  // copy under the lock; invoke outside it
    }

    // Pull any newly-decoded external frame (e.g. mpv -> GL texture) while the
    // GL context is current, BEFORE executing queued draw commands, so the
    // composited texture contents are up to date for this frame.
    if (pump != nullptr) {
      pump();
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

    // Drain one-shot render-thread tasks (e.g. external GL producer teardown).
    // Run them AFTER the pump and all queued draw commands for this iteration so
    // a task that deletes GL textures/FBOs cannot race a pending DrawTexture that
    // still references them. The caller of RunOnRenderThread blocks on the
    // promise set inside these tasks.
    std::vector<std::function<void()>> tasks;
    {
      std::scoped_lock lock(frame_mutex_);
      tasks = std::move(render_tasks_);
    }
    for (auto& t : tasks) {
      t();
    }
  }

  // Detach the WGL context from this thread. This MUST happen before the App
  // thread's glfwDestroyWindow/glfwTerminate runs, otherwise WGL blocks waiting
  // for the context that is still current on the (now-exiting) render thread.
#ifdef NEOFLUX_PLATFORM_DESKTOP
  if (glfw_bridge_ != nullptr) {
    glfw_bridge_->ReleaseContext();
  }
#endif

  LOG(INFO) << "Render thread exiting";
}

}  // namespace neoflux
