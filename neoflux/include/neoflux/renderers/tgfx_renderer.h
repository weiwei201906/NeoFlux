// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - tgfx_renderer.h
//
// tgfx-backed renderer. Replays RenderCommand objects as tgfx::Canvas draw
// calls.
//
// The GPU backend is tgfx's own COMPILE-TIME choice (tgfx's TGFX_USE_*
// switches; exactly one is active per build, resolved in
// thirdparty/CMakeLists.txt). Every platform follows the same shape: a tgfx
// Window owns the GPU device, the graphics surface/swapchain, and presents on
// context->submit(). Only window CREATION is platform-specific:
//   - Desktop OpenGL:  Linux -> tgfx::EGLWindow::MakeFrom(XID)   (X11)
//                      Win32 -> tgfx::WGLWindow::MakeFrom(HWND)
//                      Apple -> unsupported; build with TGFX_USE_METAL.
//   - Desktop Vulkan / D3D12 / Metal: a backend tgfx Window wraps the GLFW
//     native window (VulkanWindow / D3D12Window / MetalWindow).
//   - Android: tgfx::EGLWindow::MakeFrom(ANativeWindow*).
//
// GLFW is a pure window + input bridge: windows are created with GLFW_NO_API
// and NeoFlux never manages a GL/EGL/WGL context itself.
//
// Pimpl: all tgfx / GLFW state lives in struct Impl defined in the .cpp.
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

  // Binds the native window handed over by the platform layer (GLFWwindow*
  // on desktop, ANativeWindow* on Android). Real tgfx window/device creation
  // is deferred to the first BeginFrame(), which runs on the render thread.
  bool Init(int width, int height, void* native_handle = nullptr);

  // Begins a new frame: (re)creates the tgfx window/surface and clears the
  // background.
  void BeginFrame(const Color& clear_color = {255, 255, 255, 255});

  // Ends the frame: flushes and submits all tgfx recording to the GPU. The
  // tgfx Window presents the frame as part of submit(); there is no separate
  // buffer swap.
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
