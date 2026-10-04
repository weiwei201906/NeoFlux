// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - tgfx_renderer.h
//
// tgfx-backed renderer. Replays RenderCommand objects as tgfx::Canvas draw
// calls.
//
// The GPU backend is a COMPILE-TIME choice (-DNEOFLUX_BACKEND=<gl|vulkan|d3d12|
// metal>; tgfx allows only one per build):
//   - gl (default): GLFW creates a GL context and this renderer attaches to it
//     via tgfx::GLDevice::Current(), drawing into the default framebuffer that
//     GLFW swaps.
//   - vulkan/d3d12/metal: the backend device is created on the render thread and
//     wrapped in a tgfx::Window, which owns the swapchain; Surface::MakeFrom(
//     context, window) + context->submit() present the frame. GLFW is created
//     with GLFW_NO_API and does not swap buffers.
//
// Pimpl: all tgfx / GL / GLFW state lives in struct Impl defined in the .cpp.
// The public header exposes no third-party types.
// =============================================================================

#ifndef NEOFLUX_RENDER_TGFX_RENDERER_H_
#define NEOFLUX_RENDER_TGFX_RENDERER_H_

#include <memory>

#include "neoflux/core/noncopyable.h"
#include "neoflux/core/types.h"
#include "neoflux/renderers/render_command.h"

namespace neoflux {

// tgfx-Canvas renderer that executes render commands.
class TgfxRenderer : public NonCopyable {
 public:
  TgfxRenderer();
  ~TgfxRenderer();

  // Binds the surface and (desktop) the GLFW window whose WGL context is
  // already current on the calling thread. Real tgfx device creation is
  // deferred to the first BeginFrame(), which runs on the render thread.
  bool Init(int width, int height, void* native_handle = nullptr);

  // Begins a new frame: (re)creates the tgfx device/surface for the current
  // framebuffer size and clears the background.
  void BeginFrame(const Color& clear_color = {255, 255, 255, 255});

  // Ends the frame: flushes and submits all tgfx recording to the GL context.
  // Buffer swap is performed separately by the GLFW bridge.
  void EndFrame();

  // Replays a single render command on the current canvas.
  void Execute(const RenderCommand& command);

  // Resizes the logical surface dimensions.
  void Resize(int width, int height);

  // Returns the logical surface width/height.
  [[nodiscard]] int GetWidth() const noexcept;
  [[nodiscard]] int GetHeight() const noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace neoflux

#endif  // NEOFLUX_RENDER_TGFX_RENDERER_H_
