// =============================================================================
// NeoFlux - tgfx_renderer.h
//
// tgfx-based renderer. Executes RenderCommand objects by translating them
// into tgfx Canvas draw calls. On desktop, the GLFW bridge provides the
// window and GL context; tgfx performs all actual rendering (geometry,
// text with UTF-8 support, clipping, transforms).
//
// All tgfx objects are held as opaque pointers to avoid leaking tgfx
// headers into the public API.
// =============================================================================

#ifndef NEOFLUX_RENDER_TGFX_RENDERER_H_
#define NEOFLUX_RENDER_TGFX_RENDERER_H_

#include <memory>
#include <string_view>

#include "neoflux/core/noncopyable.h"
#include "neoflux/core/types.h"
#include "neoflux/render/render_command.h"

namespace neoflux {

// Forward declaration of the platform-specific renderer implementation.
// Defined in tgfx_renderer.cpp (GlRendererImpl for desktop/mobile).
class GlRendererImpl;

// tgfx-backed renderer that executes render commands.
class TgfxRenderer : public NonCopyable {
 public:
  TgfxRenderer();
  ~TgfxRenderer();

  // Initializes the renderer for a surface of the given dimensions.
  // On desktop, `native_handle` is the GLFW window pointer.
  // `font_dir` is the directory to scan for font files (.ttf/.otf/.ttc).
  bool Init(int width, int height, std::string_view font_dir,
            void* native_handle = nullptr);

  // Begins a new frame. Clears the background.
  void BeginFrame(const Color& clear_color = {255, 255, 255, 255});

  // Ends the current frame and submits it to the GPU.
  void EndFrame();

  // Executes a single render command.
  void Execute(const RenderCommand& command);

  // Resizes the render surface.
  void Resize(int width, int height);

  // Returns the surface width.
  [[nodiscard]] int GetWidth() const noexcept;

  // Returns the surface height.
  [[nodiscard]] int GetHeight() const noexcept;

 private:
  std::unique_ptr<GlRendererImpl> impl_;
  int width_ = 0;
  int height_ = 0;
  bool initialized_ = false;
};

}  // namespace neoflux

#endif  // NEOFLUX_RENDER_TGFX_RENDERER_H_
