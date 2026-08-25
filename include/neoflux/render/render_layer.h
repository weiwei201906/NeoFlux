// =============================================================================
// NeoFlux - render_layer.h
//
// Render layer: consumes RenderCommands from the SPSC ring queue and
// executes them using the platform-appropriate rendering backend.
// All method implementations are in render_layer.cpp.
// =============================================================================

#ifndef NEOFLUX_RENDER_RENDER_LAYER_H_
#define NEOFLUX_RENDER_RENDER_LAYER_H_

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <future>
#include <memory>
#include <mutex>
#include <string_view>
#include <thread>

#include "neoflux/core/noncopyable.h"
#include "neoflux/core/ring_queue.h"
#include "neoflux/core/types.h"
#include "neoflux/render/platform_bridge.h"
#include "neoflux/render/render_command.h"

namespace neoflux {

// Forward declarations.
class TgfxRenderer;
class GlfwBridge;

// The render layer owns the render thread and executes render commands.
class RenderLayer : public NonCopyable {
 public:
  RenderLayer();
  ~RenderLayer();

  // Starts the render thread and initializes the rendering backend.
  //
  // On desktop, `platform_surface` is ignored and a GLFW window is created.
  // On mobile, `platform_surface` must be the native surface handle
  // (ANativeWindow* on Android, CAMetalLayer* on iOS); tgfx renders
  // directly into this surface without any windowing bridge.
  // `font_dir` is the directory to scan for font files (.ttf/.otf/.ttc).
  bool Start(int width, int height, std::string_view title,
             std::string_view font_dir,
             void* platform_surface = nullptr);

  // Stops the render thread and shuts down the rendering backend.
  void Stop();

  // Submits a batch of render commands to the ring queue.
  std::size_t Submit(const RenderCommand* commands, std::size_t count);

  // Returns true if the render thread is running.
  [[nodiscard]] bool IsRunning() const noexcept;

  // Returns true if the window should close (desktop only; mobile always
  // returns false -- app lifecycle is managed by the OS).
  [[nodiscard]] bool ShouldClose() const;

  // Polls window events (desktop only; called from UI thread).
  void PollEvents();

  // Returns the platform bridge (may be nullptr before Start).
  [[nodiscard]] PlatformBridge* GetPlatformBridge() const noexcept;

  // Returns the actual window/framebuffer size in pixels (may differ from
  // the requested size due to DPI scaling).
  void GetWindowSize(int& width, int& height) const noexcept;

  // Notifies the render layer that the platform surface has been destroyed
  // (e.g. Android app backgrounded). Forwards to the platform bridge. The
  // render thread will skip frames until OnSurfaceCreated is called.
  void OnSurfaceDestroyed();

  // Notifies the render layer that the platform surface has been recreated
  // (e.g. Android app foregrounded). `new_surface` is the new native surface
  // handle. Returns true on success. Forwards to the platform bridge.
  bool OnSurfaceCreated(void* new_surface);

 private:
  // Main render loop. Runs on the render thread.
  void RenderLoop();

  SpscRingQueue<RenderCommand> command_queue_;

  // Condition variable to wake the render thread when a new frame is
  // submitted. Avoids busy-polling on the SPSC queue.
  std::mutex frame_mutex_{};
  std::condition_variable frame_cv_{};
  bool frame_ready_ = false;

  std::atomic<bool> running_{false};
  std::atomic<bool> should_close_{false};
  std::unique_ptr<std::thread> render_thread_ = nullptr;

  // Set by the render thread after it has made the GL context current and
  // performed a preliminary BeginFrame/EndFrame to initialise GL resources.
  // Start() blocks on this until the render thread is ready, so the first
  // real frame submitted by the application never races GL initialisation.
  std::promise<void> render_ready_{};
  std::future<void> render_ready_future_{};

  std::unique_ptr<TgfxRenderer> renderer_{nullptr};
  std::unique_ptr<PlatformBridge> platform_bridge_{nullptr};

  std::uint16_t window_width_ = 800;
  std::uint16_t window_height_ = 600;

  // True when the platform surface is valid and rendering is possible.
  // Set to false by OnSurfaceDestroyed (app backgrounded), set to true by
  // OnSurfaceCreated (app foregrounded). Render loop skips frames when false.
  std::atomic<bool> surface_valid_{true};
};

}  // namespace neoflux

#endif  // NEOFLUX_RENDER_RENDER_LAYER_H_
