// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - render_command.h
//
// Render commands are the FIFO messages passed from the Application layer
// to the Render layer via the SPSC ring queue. A flat struct is used (rather
// than std::variant) for cache efficiency in the render hot path; the `type`
// field discriminates which payload fields are valid.
//
// The struct stays a trivially copyable POD on purpose: it is memcpy'd through
// the lock-free queue, so it must not own shared state (no std::shared_ptr).
// External images are therefore referenced by an opaque id, never by value.
//
// All factory method implementations are in render_command.cpp.
// =============================================================================

#ifndef NEOFLUX_RENDER_RENDER_COMMAND_H_
#define NEOFLUX_RENDER_RENDER_COMMAND_H_

#include <cstdint>
#include <string>

#include "neoflux/core/types.h"

namespace neoflux {

// Enumeration of render command types.
enum class RenderCommandType : std::uint8_t {
  kNoop,
  kDrawRect,
  kDrawRoundedRect,
  kDrawText,
  kDrawTexture,
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
  std::string text;               // kDrawText (UTF-8)
  std::string font_name;          // kDrawText (font name, resolved by FontManager)
  Point point{};                   // kDrawText
  float font_size = 14.0F;         // kDrawText
  float translate_x = 0.0F;        // kTranslate
  float translate_y = 0.0F;        // kTranslate
  float corner_radius = 0.0F;      // kDrawRoundedRect

  // Opaque image id to composite (kDrawTexture). The command is part of the
  // backend-agnostic render protocol: the id names a CPU frame that the
  // producer (the media module) registered with the frame image registry, and
  // the active tgfx backend uploads that frame during Canvas::drawImageRect().
  //
  // ID CONTRACT:
  //   - 0 means "nothing to draw"; the renderer skips the command.
  //   - A non-zero id stays valid until its producer releases it. The renderer
  //     resolves the id at execute time, so a command that outlives its frame
  //     resolves to "not found" and is skipped instead of drawing garbage.
  //   - Ids are opaque: no backend may interpret them as a GL texture name,
  //     a VkImage handle, or any other GPU object.
  std::uint32_t image_id = 0;

  // Factory: create a draw-rect command.
  [[nodiscard]] static RenderCommand MakeDrawRect(const Rect& rect,
                                                  const Color& color);

  // Factory: create a draw-rounded-rect command.
  [[nodiscard]] static RenderCommand MakeDrawRoundedRect(const Rect& rect,
                                                         const Color& color,
                                                         float radius);

  // Factory: create a draw-text command.
  [[nodiscard]] static RenderCommand MakeDrawText(std::string text,
                                                  const Point& position,
                                                  const Color& color,
                                                  float font_size,
                                                  std::string font_name);

  // Factory: create a draw-texture command. The name is kept from the original
  // GL-texture protocol, but |image_id| is the backend-agnostic opaque image id
  // documented on RenderCommand::image_id; see also RenderContext::DrawImage(),
  // the widget-facing entry point.
  [[nodiscard]] static RenderCommand MakeDrawTexture(std::uint32_t image_id,
                                                     const Rect& rect);

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
