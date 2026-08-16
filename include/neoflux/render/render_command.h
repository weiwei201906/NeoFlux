// =============================================================================
// NeoFlux - render_command.h
//
// Render commands are the FIFO messages passed from the Application layer
// to the Render layer via the SPSC ring queue.
// All factory method implementations are in render_command.cpp.
// =============================================================================

#ifndef NEOFLUX_RENDER_RENDER_COMMAND_H_
#define NEOFLUX_RENDER_RENDER_COMMAND_H_

#include <cstdint>
#include <string>
#include <variant>

#include "neoflux/core/types.h"

namespace neoflux {

// Enumeration of render command types.
enum class RenderCommandType : uint8_t {
  kNoop,
  kDrawRect,
  kDrawText,
  kSave,
  kRestore,
  kTranslate,
  kClipRect,
  kBeginFrame,
  kEndFrame,
};

// Command payload: draw a filled rectangle.
struct DrawRectCommand {
  Rect rect;
  Color color;
};

// Command payload: draw text at a position.
struct DrawTextCommand {
  std::string text;
  Point position;
  Color color;
  float font_size = 14.0F;
};

// Command payload: translate the coordinate system.
struct TranslateCommand {
  float dx = 0.0F;
  float dy = 0.0F;
};

// Command payload: set a rectangular clip.
struct ClipRectCommand {
  Rect rect;
};

// A single render command, discriminated by type.
struct RenderCommand {
  RenderCommandType type = RenderCommandType::kNoop;
  std::variant<std::monostate, DrawRectCommand, DrawTextCommand,
               TranslateCommand, ClipRectCommand>
      payload;

  // Factory: create a draw-rect command.
  [[nodiscard]] static RenderCommand MakeDrawRect(const Rect& rect,
                                                  const Color& color);

  // Factory: create a draw-text command.
  [[nodiscard]] static RenderCommand MakeDrawText(std::string text,
                                                  const Point& position,
                                                  const Color& color,
                                                  float font_size);

  // Factory: create a save command.
  [[nodiscard]] static RenderCommand MakeSave();

  // Factory: create a restore command.
  [[nodiscard]] static RenderCommand MakeRestore();

  // Factory: create a translate command.
  [[nodiscard]] static RenderCommand MakeTranslate(float delta_x, float delta_y);

  // Factory: create a clip-rect command.
  [[nodiscard]] static RenderCommand MakeClipRect(const Rect& rect);

  // Factory: create a begin-frame command.
  [[nodiscard]] static RenderCommand MakeBeginFrame();

  // Factory: create an end-frame command.
  [[nodiscard]] static RenderCommand MakeEndFrame();
};

}  // namespace neoflux

#endif  // NEOFLUX_RENDER_RENDER_COMMAND_H_
