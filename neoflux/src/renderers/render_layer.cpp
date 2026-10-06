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

#include "native/native_tuning.h"
#include "neoflux/renderers/glfw_bridge.h"
#include "neoflux/renderers/platform_bridge.h"
#include "neoflux/core/flags.h"
#include "neoflux/renderers/render_command.h"
#include "neoflux/renderers/tgfx_renderer.h"

// render_queue_capacity and render_queue_drop_log_max are defined in
// core/flags.cpp and declared in core/flags.h.
//
// The GPU backend is NOT a runtime flag and NOT a NeoFlux concept: it is
// tgfx's own TGFX_USE_* compile-time choice, resolved to exactly one backend
// at configure time (see thirdparty/CMakeLists.txt). Switching backends
// requires a reconfigure + rebuild, not a command-line option.

namespace neoflux {

// Name of the tgfx backend compiled into this binary. thirdparty/
// CMakeLists.txt resolves tgfx's options to a single TGFX_USE_*=1 definition
// that mirrors what tgfx actually compiled.
#if defined(TGFX_USE_OPENGL)
static constexpr const char kBackendName[] = "OpenGL";
#elif defined(TGFX_USE_VULKAN)
static constexpr const char kBackendName[] = "Vulkan";
#elif defined(TGFX_USE_D3D12)
static constexpr const char kBackendName[] = "D3D12";
#elif defined(TGFX_USE_METAL)
static constexpr const char kBackendName[] = "Metal";
#else
static constexpr const char kBackendName[] = "unknown";
#endif

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

  // The backend is a compile-time choice; log which one this binary carries.
  LOG(INFO) << "RenderLayer starting: " << width << "x" << height
            << " backend=" << kBackendName << " (tgfx)";

  renderer_ = std::make_unique<TgfxRenderer>();

#ifdef NEOFLUX_PLATFORM_DESKTOP
  // Desktop: create the GLFW window (GLFW_NO_API); the tgfx Window built in
  // TgfxRenderer::EnsureDevice() owns the GPU context, surface, and
  // presentation. GLFW only handles windowing and input.
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

  // Note: renderer_->Init() already stored the logical window size for
  // u_resolution (shader layout coordinates). The actual framebuffer size
  // (which may differ due to DPI scaling) is queried each frame in
  // TgfxRenderer::BeginFrame(). Do NOT call Resize() here with the
  // framebuffer size -- that would corrupt u_resolution and make layout
  // coordinates mismatch the shader.
#else
  // Mobile: tgfx owns the EGL context and swapchain (tgfx::EGLWindow); the
  // platform bridge only carries the native window and touch input.
  if (platform_surface == nullptr) {
    LOG(ERROR) << "Mobile platform surface is required for tgfx initialization";
    return false;
  }

  mobile_bridge_ = CreateMobileBridge(platform_surface, width, height);
  if (mobile_bridge_ == nullptr) {
    LOG(ERROR) << "Failed to create the mobile platform bridge";
    return false;
  }

  if (!renderer_->Init(width, height, platform_surface)) {
    LOG(ERROR) << "Failed to initialize tgfx renderer (mobile)";
    mobile_bridge_.reset();
    return false;
  }
#endif

  running_.store(true);
  render_thread_ = std::make_unique<std::thread>([this]() { RenderLoop(); });

  // Block until the render thread is up and will service submitted frames
  // promptly. The tgfx Window/device is created lazily in BeginFrame() on the
  // render thread, so the first real frame triggers tgfx initialisation
  // (surfaces, fonts) there; the wait merely avoids the first frame racing
  // thread startup.
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

  // Wake the render thread so it can exit the wait loop. join() returns after
  // the render thread has drained its frame state, so by the time it returns
  // it is safe for the App thread to destroy the window.
  Wake();

  if (render_thread_ != nullptr && render_thread_->joinable()) {
    render_thread_->join();
  }
  render_thread_.reset();

  // Intentionally abandon the renderer rather than destroy it here. tgfx's
  // Device teardown requires a full releaseAll() protocol that this app does
  // not implement, and destroying it from a non-owning thread can deadlock on
  // the device's context lock. The process is about to exit, so we leak the
  // small host-side object and let the OS reclaim all GPU resources when the
  // window and the tgfx Window's context are destroyed below.
  // Intentional leak: unique_ptr::release() discards the pointer on purpose
  // (the OS reclaims everything at process exit; see the comment above).
  renderer_.release();  // NOLINT(bugprone-unused-return-value)

  mobile_bridge_.reset();
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
    // No render thread is alive to service the task. The window/tgfx device
    // have already been torn down, so any GPU resources the task would free
    // are reclaimed by the driver/process teardown. Drop the task rather than
    // run it on this thread (which has no render device and would itself
    // crash).
    LOG(WARNING) << "RunOnRenderThread: render thread not running, dropping task";
    return;
  }
  // Signal completion once the render thread has executed the wrapped task, so
  // the caller (App thread) blocks until teardown has actually happened.
  auto done = std::make_shared<std::promise<void>>();
  std::future<void> fut = done->get_future();
  {
    std::scoped_lock lock(frame_mutex_);
    render_tasks_.emplace_back([task = std::move(task), done]() mutable {
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
  // Mobile: the app shell sets the close flag via the bridge (surface
  // destroyed / app paused), which PlatformBridge::ShouldClose() reports.
  if (mobile_bridge_ != nullptr) {
    return mobile_bridge_->ShouldClose();
  }
  return should_close_.load();
}

void RenderLayer::PollEvents() {
#ifdef NEOFLUX_PLATFORM_DESKTOP
  if (glfw_bridge_ != nullptr) {
    glfw_bridge_->PollEvents();
  }
#else
  // Mobile: touch events are pushed asynchronously through the bridge's
  // input callback (JNI/UI thread); polling is a no-op but keeps the frame
  // loop uniform across platforms.
  if (mobile_bridge_ != nullptr) {
    mobile_bridge_->PollEvents();
  }
#endif
}

GlfwBridge* RenderLayer::GetGlfwBridge() const noexcept {
  return glfw_bridge_.get();
}

PlatformBridge* RenderLayer::GetPlatformBridge() const noexcept {
  return mobile_bridge_.get();
}

void RenderLayer::GetWindowSize(int& width, int& height) const noexcept {
#ifdef NEOFLUX_PLATFORM_DESKTOP
  if (glfw_bridge_ != nullptr) {
    glfw_bridge_->GetWindowSize(width, height);
    return;
  }
#endif
  // Mobile: the app shell owns the surface and pushes size changes into the
  // bridge with PlatformBridge::Resize() (rotation, new surface after the app
  // resumes), so the bridge is authoritative. Reporting the snapshot taken at
  // Start() instead would hand every consumer stale dimensions: notably the
  // touch-coordinate scaling in Application::DispatchPointerEvent(), which
  // would map taps outside the tree after a rotation. Desktop never reaches
  // this branch because mobile_bridge_ stays null there.
  if (mobile_bridge_ != nullptr) {
    width = mobile_bridge_->GetWidth();
    height = mobile_bridge_->GetHeight();
    return;
  }
  width = window_width_;
  height = window_height_;
}

void RenderLayer::RenderLoop() {
  // Platform-native scheduling tuning (MMCSS / priority / SCHED_FIFO / QoS)
  // and big-core pinning on hybrid topologies. Best-effort: on restricted
  // systems these silently keep default scheduling.
  native::TuneRenderThread();
  native::PinThreadToBigCores();

  LOG(INFO) << "Render thread started";

  // Signal readiness immediately: the tgfx Window/device is created lazily in
  // BeginFrame() on this thread. The first real frame from the application
  // will trigger BeginFrame() which initialises tgfx resources (fonts,
  // surfaces) on this thread.
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

    // Publish any newly-decoded external frame (e.g. an mpv CPU frame image)
    // before executing queued draw commands, so the composited frame contents
    // are up to date for this iteration.
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
            // Presentation is part of EndFrame(): the tgfx Window submits the
            // recording and flips its own swapchain. No GLFW swap here.
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

    // Drain one-shot render-thread tasks (e.g. external frame producer
    // teardown). Run them AFTER the pump and all queued draw commands for this
    // iteration so a task that releases a frame image cannot race a pending
    // draw command that still references it. The caller of RunOnRenderThread
    // blocks on the promise set inside these tasks.
    std::vector<std::function<void()>> tasks;
    {
      std::scoped_lock lock(frame_mutex_);
      tasks = std::move(render_tasks_);
    }
    for (auto& t : tasks) {
      t();
    }
  }

  LOG(INFO) << "Render thread exiting";
}

}  // namespace neoflux
