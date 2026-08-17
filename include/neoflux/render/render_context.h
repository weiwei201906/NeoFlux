// =============================================================================
// NeoFlux - render_context.h
//
// RenderContext is the drawing interface passed to Widget::Paint().
// All method implementations are in render_context.cpp.
// =============================================================================

#ifndef NEOFLUX_RENDER_RENDER_CONTEXT_H_
#define NEOFLUX_RENDER_RENDER_CONTEXT_H_

#include <cstddef>
#include <string_view>
#include <vector>

#include "neoflux/core/types.h"
#include "neoflux/render/render_command.h"

namespace neoflux {

// Drawing context for widget painting.
//
// Records render commands into an internal buffer. After the widget tree
// finishes painting, the command buffer is submitted to the render layer.
class RenderContext {
 public:
  RenderContext();
  ~RenderContext() = default;

  // Draws a filled rectangle.
  void DrawRect(const Rect& rect, const Color& color);

  // Draws text at the given position.
  void DrawText(std::string_view text, const Point& position,
                const Color& color, float font_size,
                std::string_view font_name = "");

  // Saves the current transform/clip state.
  void Save();

  // Restores the previous transform/clip state.
  void Restore();

  // Translates the coordinate origin by (delta_x, delta_y).
  void Translate(float delta_x, float delta_y);

  // Sets a rectangular clip region (intersects with current clip).
  void ClipRect(const Rect& rect);

  // Returns a read-only reference to the recorded commands.
  [[nodiscard]] const std::vector<RenderCommand>& GetCommands() const noexcept;

  // Clears all recorded commands.
  void Clear() noexcept;

  // Returns the number of recorded commands.
  [[nodiscard]] std::size_t GetCommandCount() const noexcept;

  // Appends a pre-built render command (used for frame boundaries).
  void AppendCommand(RenderCommand command);

 private:
  std::vector<RenderCommand> commands_;
};

}  // namespace neoflux

#endif  // NEOFLUX_RENDER_RENDER_CONTEXT_H_
