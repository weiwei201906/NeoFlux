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
  void DrawRectImpl(const Rect& rect, const Color& color);
  void DrawTextImpl(std::string_view text, const Point& position,
                    const Color& color, float font_size,
                    std::string_view font_name);
  void SaveImpl();
  void RestoreImpl();
  void TranslateImpl(float delta_x, float delta_y);
  void ClipRectImpl(const Rect& rect);

  // Opaque tgfx objects (owned by this renderer).
  void* device_ = nullptr;       // tgfx::Device subclass (GLFW).
  void* window_ = nullptr;       // tgfx::Window subclass (GLFW).
  void* context_ = nullptr;      // tgfx::Context* (locked during frame).
  void* surface_ = nullptr;      // shared_ptr<tgfx::Surface>.
  void* canvas_ = nullptr;       // tgfx::Canvas*.
  void* typeface_ = nullptr;     // shared_ptr<tgfx::Typeface>.
  void* impl_ = nullptr;         // GlRendererImpl (fallback path).
  int width_ = 0;
  int height_ = 0;
  bool initialized_ = false;
};

}  // namespace neoflux

#endif  // NEOFLUX_RENDER_TGFX_RENDERER_H_
