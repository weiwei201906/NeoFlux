// =============================================================================
// NeoFlux - render_context.cpp
//
// Implementation of RenderContext methods moved from header.
// =============================================================================

#include "neoflux/render/render_context.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include "neoflux/core/types.h"

namespace neoflux {

RenderContext::RenderContext() = default;

void RenderContext::DrawRect(const Rect& rect, const Color& color) {
  commands_.push_back(RenderCommand::MakeDrawRect(rect, color));
}

void RenderContext::DrawRoundedRect(const Rect& rect, const Color& color,
                                    float radius) {
  commands_.push_back(RenderCommand::MakeDrawRoundedRect(rect, color, radius));
}

void RenderContext::DrawText(std::string_view text, const Point& position,
                             const Color& color, float font_size,
                             std::string_view font_name) {
  commands_.push_back(RenderCommand::MakeDrawText(
      std::string(text), position, color, font_size, std::string(font_name)));
}

void RenderContext::DrawTexture(std::uint32_t texture_id, const Rect& rect) {
  commands_.push_back(RenderCommand::MakeDrawTexture(texture_id, rect));
}

void RenderContext::Save() {
  commands_.push_back(RenderCommand::MakeSave());
}

void RenderContext::Restore() {
  commands_.push_back(RenderCommand::MakeRestore());
}

void RenderContext::Translate(float delta_x, float delta_y) {
  commands_.push_back(RenderCommand::MakeTranslate(delta_x, delta_y));
}

void RenderContext::ClipRect(const Rect& rect) {
  commands_.push_back(RenderCommand::MakeClipRect(rect));
}

const std::vector<RenderCommand>& RenderContext::GetCommands()
    const noexcept {
  return commands_;
}

void RenderContext::Clear() noexcept { commands_.clear(); }

std::size_t RenderContext::GetCommandCount() const noexcept {
  return commands_.size();
}

void RenderContext::AppendCommand(RenderCommand command) {
  commands_.push_back(std::move(command));
}

}  // namespace neoflux
