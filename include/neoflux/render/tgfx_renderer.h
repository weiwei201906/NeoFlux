// =============================================================================
// NeoFlux - tgfx_renderer.h
//
// tgfx-based renderer. All method implementations are in tgfx_renderer.cpp.
// =============================================================================

#ifndef NEOFLUX_RENDER_TGFX_RENDERER_H_
#define NEOFLUX_RENDER_TGFX_RENDERER_H_

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
  bool Init(int width, int height, void* native_handle = nullptr);

  // Begins a new frame. Clears the background.
  void BeginFrame(const Color& clear_color = {255, 255, 255, 255});

  // Ends the current frame and presents it to the surface.
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
                    const Color& color, float font_size);
  void SaveImpl();
  void RestoreImpl();
  void TranslateImpl(float delta_x, float delta_y);
  void ClipRectImpl(const Rect& rect);

  int width_ = 0;
  int height_ = 0;
  bool initialized_ = false;
  void* tgfx_context_ = nullptr;
};

}  // namespace neoflux

#endif  // NEOFLUX_RENDER_TGFX_RENDERER_H_
