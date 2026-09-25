// =============================================================================
// NeoFlux - tgfx_renderer.cpp
//
// Renderer backend facade. The actual rendering work is delegated to the
// platform implementation in src/native/gl_renderer.cpp (GlRendererImpl),
// which owns all GL/FreeType/GLFW details behind a PImpl.
//
// TgfxRenderer translates RenderCommand objects into GlRendererImpl calls.
// This keeps the command interface independent of the concrete backend so
// that the GL implementation can be swapped for a real tgfx backend later
// without touching the rest of the framework.
//
// All method implementations live in .cpp (header declares only).
// =============================================================================

#include "neoflux/render/tgfx_renderer.h"

#include <memory>
#include <string_view>

#include "neoflux/native/gl_renderer.h"

namespace neoflux {

TgfxRenderer::TgfxRenderer() = default;

TgfxRenderer::~TgfxRenderer() = default;

bool TgfxRenderer::Init(int width, int height, std::string_view font_dir,
                        void* native_handle) {
  if (initialized_) {
    return true;
  }
#if defined(NEOFLUX_PLATFORM_DESKTOP) || defined(NEOFLUX_PLATFORM_MOBILE)
  auto impl = std::make_unique<GlRendererImpl>();
  if (!impl->Init(width, height, font_dir, native_handle)) {
    return false;
  }
  impl_ = std::move(impl);
  width_ = width;
  height_ = height;
  initialized_ = true;
  return true;
#else
  (void)width;
  (void)height;
  (void)font_dir;
  (void)native_handle;
  return false;
#endif
}

void TgfxRenderer::BeginFrame(const Color& clear_color) {
#if defined(NEOFLUX_PLATFORM_DESKTOP) || defined(NEOFLUX_PLATFORM_MOBILE)
  if (impl_ != nullptr) {
    impl_->BeginFrame(clear_color);
  }
#else
  (void)clear_color;
#endif
}

void TgfxRenderer::EndFrame() {
#if defined(NEOFLUX_PLATFORM_DESKTOP) || defined(NEOFLUX_PLATFORM_MOBILE)
  if (impl_ != nullptr) {
    impl_->EndFrame();
  }
#endif
}

void TgfxRenderer::Execute(const RenderCommand& command) {
#if defined(NEOFLUX_PLATFORM_DESKTOP) || defined(NEOFLUX_PLATFORM_MOBILE)
  if (impl_ == nullptr) {
    return;
  }
  // Each case dispatches to a distinct renderer method; clang-tidy's
  // branch-clone check flags the similar structure but the calls differ.
  switch (command.type) {  // NOLINT(bugprone-branch-clone)
    case RenderCommandType::kDrawRect:
      impl_->DrawRect(command.rect, command.color);
      break;
    case RenderCommandType::kDrawRoundedRect:
      impl_->DrawRoundedRect(command.rect, command.color,
                             command.corner_radius);
      break;
    case RenderCommandType::kDrawText:
      impl_->DrawText(command.text, command.point, command.color,
                      command.font_size, command.font_name);
      break;
    case RenderCommandType::kDrawTexture:
      impl_->DrawTexture(command.texture_id, command.rect);
      break;
    case RenderCommandType::kSave:
      impl_->Save();
      break;
    case RenderCommandType::kRestore:
      impl_->Restore();
      break;
    case RenderCommandType::kTranslate:
      impl_->Translate(command.translate_x, command.translate_y);
      break;
    case RenderCommandType::kClipRect:
      impl_->ClipRect(command.rect);
      break;
    default:
      break;
  }
#else
  (void)command;
#endif
}

void TgfxRenderer::Resize(int width, int height) {
  width_ = width;
  height_ = height;
#if defined(NEOFLUX_PLATFORM_DESKTOP) || defined(NEOFLUX_PLATFORM_MOBILE)
  if (impl_ != nullptr) {
    impl_->Resize(width, height);
  }
#endif
}

int TgfxRenderer::GetWidth() const noexcept { return width_; }

int TgfxRenderer::GetHeight() const noexcept { return height_; }

}  // namespace neoflux
