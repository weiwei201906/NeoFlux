// =============================================================================
// NeoFlux - render_command.h
//
// Render commands are the FIFO messages passed from the Application layer
// to the Render layer via the SPSC ring queue. A flat struct is used (rather
// than std::variant) for cache efficiency in the render hot path; the `type`
// field discriminates which payload fields are valid.
// All factory method implementations are in render_command.cpp.
// =============================================================================

#ifndef NEOFLUX_RENDER_RENDER_COMMAND_H_
#define NEOFLUX_RENDER_RENDER_COMMAND_H_

#include <cstdint>
#include <string>

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

// A single render command. Fields are interpreted according to `type`.
struct RenderCommand {
  RenderCommandType type = RenderCommandType::kNoop;

  // Payload fields (valid depending on `type`).
  Rect rect{};                     // kDrawRect, kClipRect
  Color color{};                   // kDrawRect, kDrawText
  std::string text{};              // kDrawText (UTF-8)
  Point point{};                   // kDrawText
  float font_size = 14.0F;         // kDrawText
  float translate_x = 0.0F;        // kTranslate
  float translate_y = 0.0F;        // kTranslate

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
  [[nodiscard]] static RenderCommand MakeTranslate(float delta_x,
                                                   float delta_y);

  // Factory: create a clip-rect command.
  [[nodiscard]] static RenderCommand MakeClipRect(const Rect& rect);

  // Factory: create a begin-frame command.
  [[nodiscard]] static RenderCommand MakeBeginFrame();

  // Factory: create an end-frame command.
  [[nodiscard]] static RenderCommand MakeEndFrame();
};

}  // namespace neoflux

#endif  // NEOFLUX_RENDER_RENDER_COMMAND_H_
