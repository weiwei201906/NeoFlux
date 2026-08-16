// =============================================================================
// NeoFlux - render_command.cpp
//
// Implementation of RenderCommand factory methods moved from header.
// =============================================================================

#include "neoflux/render/render_command.h"

#include <utility>

namespace neoflux {

RenderCommand RenderCommand::MakeDrawRect(const Rect& rect,
                                          const Color& color) {
  RenderCommand cmd;
  cmd.type = RenderCommandType::kDrawRect;
  cmd.payload = DrawRectCommand{rect, color};
  return cmd;
}

RenderCommand RenderCommand::MakeDrawText(std::string text,
                                          const Point& position,
                                          const Color& color,
                                          float font_size) {
  RenderCommand cmd;
  cmd.type = RenderCommandType::kDrawText;
  cmd.payload =
      DrawTextCommand{std::move(text), position, color, font_size};
  return cmd;
}

RenderCommand RenderCommand::MakeSave() {
  RenderCommand cmd;
  cmd.type = RenderCommandType::kSave;
  return cmd;
}

RenderCommand RenderCommand::MakeRestore() {
  RenderCommand cmd;
  cmd.type = RenderCommandType::kRestore;
  return cmd;
}

RenderCommand RenderCommand::MakeTranslate(float dx, float dy) {
  RenderCommand cmd;
  cmd.type = RenderCommandType::kTranslate;
  cmd.payload = TranslateCommand{dx, dy};
  return cmd;
}

RenderCommand RenderCommand::MakeClipRect(const Rect& rect) {
  RenderCommand cmd;
  cmd.type = RenderCommandType::kClipRect;
  cmd.payload = ClipRectCommand{rect};
  return cmd;
}

RenderCommand RenderCommand::MakeBeginFrame() {
  RenderCommand cmd;
  cmd.type = RenderCommandType::kBeginFrame;
  return cmd;
}

RenderCommand RenderCommand::MakeEndFrame() {
  RenderCommand cmd;
  cmd.type = RenderCommandType::kEndFrame;
  return cmd;
}

}  // namespace neoflux
