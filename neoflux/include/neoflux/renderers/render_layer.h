// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
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
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string_view>
#include <thread>
#include <vector>

#include "neoflux/core/noncopyable.h"
#include "neoflux/core/ring_queue.h"
#include "neoflux/core/types.h"
#include "neoflux/renderers/render_command.h"

namespace neoflux {

// Forward declarations.
class TgfxRenderer;
class GlfwBridge;
class PlatformBridge;

// The render layer owns the render thread and executes render commands.
class RenderLayer : public NonCopyable {  // NOLINT(cppcoreguidelines-special-member-functions)
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

  // Wakes the render thread without submitting commands. Thread-safe. Used by
  // external frame producers (e.g. the mpv media player, which signals frame
  // availability on its own internal thread) to ask the render thread to wake
  // and publish a newly decoded frame via the registered render pump.
  void Wake();

  // Registers a callback invoked on the render thread at the top of every
  // render-loop wake, before queued commands are executed. Used by external
  // frame producers (e.g. the media player) to turn a decoded frame into a
  // drawable image on the thread that serializes rendering. The callback must be
  // non-blocking and must not throw. At most one pump is supported; a later call
  // replaces the earlier one. Set once at wiring time. Pass nullptr to clear.
  void SetRenderPump(std::function<void()> pump);

  // Runs |task| on the render thread and BLOCKS the calling thread until it has
  // executed. The task is serialized with the render pump and draw commands: it
  // runs at the end of a render-loop iteration, AFTER any in-flight pump and
  // AFTER all queued draw commands for that iteration have executed, so it is
  // safe to release render resources (frame images, render contexts) that
  // pending draw commands might still reference. Used by external frame
  // producers (e.g. the media player) to tear down their render state on the
  // thread that drives rendering. Must NOT be called from the render thread
  // itself (would deadlock). If the render thread is not running the task is
  // dropped (the run loop has already stopped, so nothing can reference those
  // resources any more).
  void RunOnRenderThread(std::function<void()> task);

  // Returns true if the render thread is running.
  [[nodiscard]] bool IsRunning() const noexcept;

  // Returns true if the window should close (desktop only).
  [[nodiscard]] bool ShouldClose() const;

  // Polls window events (desktop only; called from UI thread).
  void PollEvents();

  // Returns the GLFW bridge (desktop only, may be nullptr before Start).
  [[nodiscard]] GlfwBridge* GetGlfwBridge() const noexcept;

  // Returns the mobile platform bridge (mobile only, may be nullptr on
  // desktop or before Start). Touch input arrives via its SetInputCallback.
  [[nodiscard]] PlatformBridge* GetPlatformBridge() const noexcept;

  // Returns the actual window/framebuffer size in pixels (may differ from
  // the requested size due to DPI scaling).
  void GetWindowSize(int& width, int& height) const noexcept;

 private:
  // Main render loop. Runs on the render thread.
  void RenderLoop();

  SpscRingQueue<RenderCommand> command_queue_;

  // Condition variable to wake the render thread when a new frame is
  // submitted or an external producer (e.g. mpv) signals a new frame. Avoids
  // busy-polling on the SPSC queue.
  std::mutex frame_mutex_;
  std::condition_variable frame_cv_;
  bool frame_ready_ = false;
  // External frame pump (e.g. the media player's UpdateFrame), invoked on the
  // render thread at the top of each wake. Read/written under frame_mutex_ so
  // the render loop copies it out before invoking.
  std::function<void()> render_pump_;

  // One-shot tasks submitted via RunOnRenderThread and drained on the render
  // thread at the end of each loop iteration (after the pump and queued draws).
  // Guarded by frame_mutex_.
  std::vector<std::function<void()>> render_tasks_;

  std::atomic<bool> running_{false};
  std::atomic<bool> should_close_{false};
  std::unique_ptr<std::thread> render_thread_ = nullptr;

  // Set by the render thread after it has locked the tgfx context and
  // performed a preliminary BeginFrame/EndFrame to initialise the render
  // resources. Start() blocks on this until the render thread is ready, so the
  // first real frame submitted by the application never races initialisation.
  std::promise<void> render_ready_;
  std::future<void> render_ready_future_;

  std::unique_ptr<TgfxRenderer> renderer_;
  std::unique_ptr<GlfwBridge> glfw_bridge_;
  // Mobile only: input/surface bridge (Android: ANativeWindow; the rendering
  // context itself is owned by tgfx::EGLWindow). Null on desktop builds.
  std::unique_ptr<PlatformBridge> mobile_bridge_;

  int window_width_ = 800;
  int window_height_ = 600;
};

}  // namespace neoflux

#endif  // NEOFLUX_RENDER_RENDER_LAYER_H_
