// =============================================================================
// NeoFlux - tgfx_renderer.h
//
// tgfx-backed renderer. Replays RenderCommand objects as tgfx::Canvas draw
// calls. On desktop the GLFW bridge creates the window and a WGL OpenGL
// context; this renderer hands that already-current context to tgfx's OpenGL
// (WGL) backend via tgfx::GLDevice::Current() and draws into the default
// framebuffer (id 0). GLFW owns buffer swap.
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
