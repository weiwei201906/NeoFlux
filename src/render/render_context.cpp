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

void RenderContext::DrawText(std::string_view text, const Point& position,
                             const Color& color, float font_size) {
  commands_.push_back(RenderCommand::MakeDrawText(std::string(text), position,
                                                  color, font_size));
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

}  // namespace neoflux
