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
#include <cstddef>
#include <memory>
#include <string_view>
#include <thread>

#include "neoflux/core/noncopyable.h"
#include "neoflux/core/ring_queue.h"
#include "neoflux/core/types.h"
#include "neoflux/render/render_command.h"

namespace neoflux {

// Forward declarations.
class TgfxRenderer;
class GlfwBridge;

// Capacity of the render command ring queue.
inline constexpr std::size_t kRenderQueueCapacity = 2048;

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
  bool Start(int width, int height, std::string_view title,
             void* platform_surface = nullptr);

  // Stops the render thread and shuts down the rendering backend.
  void Stop();

  // Submits a batch of render commands to the ring queue.
  std::size_t Submit(const RenderCommand* commands, std::size_t count);

  // Returns true if the render thread is running.
  [[nodiscard]] bool IsRunning() const noexcept;

  // Returns true if the window should close (desktop only).
  [[nodiscard]] bool ShouldClose() const;

  // Polls window events (desktop only; called from UI thread).
  void PollEvents();

 private:
  // Main render loop. Runs on the render thread.
  void RenderLoop();

  // Executes a single render command using the active renderer.
  void ExecuteCommand(const RenderCommand& command);

  // Processes all pending commands from the ring queue.
  void ProcessPendingCommands();

  SpscRingQueue<RenderCommand, kRenderQueueCapacity> command_queue_{};

  std::atomic<bool> running_{false};
  std::atomic<bool> should_close_{false};
  std::unique_ptr<std::thread> render_thread_ = nullptr;

  std::unique_ptr<TgfxRenderer> renderer_;
  std::unique_ptr<GlfwBridge> glfw_bridge_;

  int window_width_ = 800;
  int window_height_ = 600;
};

}  // namespace neoflux

#endif  // NEOFLUX_RENDER_RENDER_LAYER_H_
