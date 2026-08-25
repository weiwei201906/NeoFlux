// =============================================================================
// NeoFlux - render_command.cpp
//
// Implementation of RenderCommand factory methods.
// =============================================================================

#include "neoflux/render/render_command.h"

#include <string>
#include <utility>

#include "neoflux/core/types.h"

namespace neoflux {

RenderCommand RenderCommand::MakeDrawRect(const Rect& rect,
                                          const Color& color) {
  RenderCommand cmd;
  cmd.type = RenderCommandType::kDrawRect;
  cmd.rect = rect;
  cmd.color = color;
  return cmd;
}

RenderCommand RenderCommand::MakeDrawRoundedRect(const Rect& rect,
                                                 const Color& color,
                                                 float radius) {
  RenderCommand cmd;
  cmd.type = RenderCommandType::kDrawRoundedRect;
  cmd.rect = rect;
  cmd.color = color;
  cmd.corner_radius = radius;
  return cmd;
}

RenderCommand RenderCommand::MakeDrawText(std::string text,
                                          const Point& position,
                                          const Color& color,
                                          float font_size,
                                          std::string_view font_name) {
  RenderCommand cmd;
  cmd.type = RenderCommandType::kDrawText;
  cmd.text = std::move(text);
  cmd.point = position;
  cmd.color = color;
  cmd.font_size = font_size;
  cmd.font_name = font_name;
  return cmd;
}

RenderCommand RenderCommand::MakeDrawTexture(std::uint32_t texture_id,
                                             const Rect& rect) {
  RenderCommand cmd;
  cmd.type = RenderCommandType::kDrawTexture;
  cmd.texture_id = texture_id;
  cmd.rect = rect;
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

RenderCommand RenderCommand::MakeTranslate(float delta_x, float delta_y) {
  RenderCommand cmd;
  cmd.type = RenderCommandType::kTranslate;
  cmd.translate_x = delta_x;
  cmd.translate_y = delta_y;
  return cmd;
}

RenderCommand RenderCommand::MakeClipRect(const Rect& rect) {
  RenderCommand cmd;
  cmd.type = RenderCommandType::kClipRect;
  cmd.rect = rect;
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
