// =============================================================================
// NeoFlux - gl_renderer.h
//
// Lightweight OpenGL fallback renderer (used when tgfx is not enabled).
// All platform-specific details (GL context, FreeType, GLFW) live in
// gl_renderer.cpp behind a PImpl. The header exposes only the command
// interface so the rest of the framework stays free of GL includes.
//
// Supported platforms:
//   Desktop: GLFW window + OpenGL 3.3 core profile.
//   Android: EGL + OpenGL ES 3.0.
//   iOS:     EAGL context + OpenGL ES (functions resolved via dlsym).
// =============================================================================

#ifndef NEOFLUX_NATIVE_GL_RENDERER_H_
#define NEOFLUX_NATIVE_GL_RENDERER_H_

#include <cstdint>
#include <memory>
#include <string_view>

#include "neoflux/core/noncopyable.h"
#include "neoflux/core/types.h"

namespace neoflux {

// OpenGL fallback renderer implementation.
//
// Provides colored / rounded-rectangle drawing, UTF-8 text rendering via a
// FreeType glyph atlas, an external texture path (for libmpv video), and
// transform / clip stacks. All GL calls run on the render thread where the
// GL context is current; BeginFrame() lazily initializes GL on first use.
class GlRendererImpl : public NonCopyable {
 public:
  GlRendererImpl();
  ~GlRendererImpl();

  // Initializes the renderer for a surface of the given dimensions.
  // `font_dir` is scanned for .ttf/.otf/.ttc files (with ../ and ../../
  // fallbacks). `native_handle` is the platform window pointer
  // (GLFWwindow* on desktop, ANativeWindow* / UIView* on mobile).
  bool Init(int width, int height, std::string_view font_dir,
            void* native_handle);

  // Clears the background and prepares state for a new frame. Lazily
  // initializes GL and preloads common ASCII glyphs on first call.
  void BeginFrame(const Color& clear_color);

  // Ends the current frame. Buffer swap is owned by the platform bridge.
  void EndFrame();

  // Draws a filled rectangle.
  void DrawRect(const Rect& rect, const Color& color);

  // Draws a filled rounded rectangle (triangle fan boundary sampling).
  void DrawRoundedRect(const Rect& rect, const Color& color, float radius);

  // Draws UTF-8 text at the given position.
  void DrawText(std::string_view text, const Point& position,
                const Color& color, float font_size,
                std::string_view font_name);

  // Draws an external GL texture (e.g. libmpv video frame) into `rect`,
  // flipping the Y axis to convert bottom-left video origin to top-left UI.
  void DrawTexture(std::uint32_t texture_id, const Rect& rect);

  // Saves the current transform / clip state.
  void Save();

  // Restores the previous transform / clip state.
  void Restore();

  // Translates the current transform by (delta_x, delta_y).
  void Translate(float delta_x, float delta_y);

  // Intersects the current clip region with `rect` and applies it via
  // glScissor.
  void ClipRect(const Rect& rect);

  // Resizes the logical surface dimensions.
  void Resize(int width, int height);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace neoflux

#endif  // NEOFLUX_NATIVE_GL_RENDERER_H_
