// =============================================================================
// NeoFlux - tgfx_renderer.cpp
//
// Implementation of TgfxRenderer. Methods moved from header.
// =============================================================================

#include "neoflux/render/tgfx_renderer.h"

#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include <glog/logging.h>

#include "neoflux/core/types.h"
#include "neoflux/render/render_command.h"

#if __has_include(<tgfx/tgfx.h>)
#include <tgfx/tgfx.h>
#define NEOFLUX_TGFX_AVAILABLE 1
#else
#define NEOFLUX_TGFX_AVAILABLE 0
#pragma message("tgfx headers not found - renderer will use stub implementation")
#endif

namespace neoflux {

#if NEOFLUX_TGFX_AVAILABLE

struct TgfxContext {
  std::shared_ptr<tgfx::Surface> surface;
  std::shared_ptr<tgfx::Canvas> canvas;
  std::shared_ptr<tgfx::Context> context;
};

TgfxRenderer::TgfxRenderer() = default;

TgfxRenderer::~TgfxRenderer() {
  if (tgfx_context_ != nullptr) {
    delete static_cast<TgfxContext*>(tgfx_context_);
    tgfx_context_ = nullptr;
  }
}

bool TgfxRenderer::Init(int width, int height, void* native_handle) {
  if (initialized_) {
    LOG(WARNING) << "TgfxRenderer already initialized";
    return false;
  }

  width_ = width;
  height_ = height;

  auto* ctx = new TgfxContext();
  ctx->context = tgfx::Context::Make(tgfx::Backend::OpenGL);
  if (ctx->context == nullptr) {
    LOG(ERROR) << "Failed to create tgfx context";
    delete ctx;
    return false;
  }

  ctx->surface =
      tgfx::Surface::MakeFrom(ctx->context, native_handle, width, height);
  if (ctx->surface == nullptr) {
    LOG(ERROR) << "Failed to create tgfx surface";
    delete ctx;
    return false;
  }

  ctx->canvas = ctx->surface->getCanvas();
  tgfx_context_ = ctx;
  initialized_ = true;

  LOG(INFO) << "TgfxRenderer initialized: " << width << "x" << height;
  return true;
}

void TgfxRenderer::BeginFrame(const Color& clear_color) {
  auto* ctx = static_cast<TgfxContext*>(tgfx_context_);
  if (ctx == nullptr || ctx->canvas == nullptr) {
    return;
  }

  tgfx::Color tgfx_color;
  tgfx_color.red = static_cast<float>(clear_color.r) / 255.0F;
  tgfx_color.green = static_cast<float>(clear_color.g) / 255.0F;
  tgfx_color.blue = static_cast<float>(clear_color.b) / 255.0F;
  tgfx_color.alpha = static_cast<float>(clear_color.a) / 255.0F;

  ctx->canvas->clear(tgfx_color);
}

void TgfxRenderer::EndFrame() {
  auto* ctx = static_cast<TgfxContext*>(tgfx_context_);
  if (ctx == nullptr || ctx->surface == nullptr) {
    return;
  }
  ctx->surface->flushAndSubmit();
}

void TgfxRenderer::Execute(const RenderCommand& command) {
  switch (command.type) {
    case RenderCommandType::kDrawRect: {
      const auto& payload = std::get<DrawRectCommand>(command.payload);
      DrawRectImpl(payload.rect, payload.color);
      break;
    }
    case RenderCommandType::kDrawText: {
      const auto& payload = std::get<DrawTextCommand>(command.payload);
      DrawTextImpl(payload.text, payload.position, payload.color,
                   payload.font_size);
      break;
    }
    case RenderCommandType::kSave:
      SaveImpl();
      break;
    case RenderCommandType::kRestore:
      RestoreImpl();
      break;
    case RenderCommandType::kTranslate: {
      const auto& payload = std::get<TranslateCommand>(command.payload);
      TranslateImpl(payload.dx, payload.dy);
      break;
    }
    case RenderCommandType::kClipRect: {
      const auto& payload = std::get<ClipRectCommand>(command.payload);
      ClipRectImpl(payload.rect);
      break;
    }
    default:
      break;
  }
}

void TgfxRenderer::Resize(int width, int height) {
  width_ = width;
  height_ = height;
  VLOG(1) << "TgfxRenderer resized: " << width << "x" << height;
}

int TgfxRenderer::GetWidth() const noexcept { return width_; }

int TgfxRenderer::GetHeight() const noexcept { return height_; }

void TgfxRenderer::DrawRectImpl(const Rect& rect, const Color& color) {
  auto* ctx = static_cast<TgfxContext*>(tgfx_context_);
  if (ctx == nullptr || ctx->canvas == nullptr) {
    return;
  }

  tgfx::Paint paint;
  paint.setColor(tgfx::Color{
      static_cast<float>(color.r) / 255.0F,
      static_cast<float>(color.g) / 255.0F,
      static_cast<float>(color.b) / 255.0F,
      static_cast<float>(color.a) / 255.0F,
  });

  tgfx::Rect tgfx_rect;
  tgfx_rect.left = rect.x;
  tgfx_rect.top = rect.y;
  tgfx_rect.right = rect.right();
  tgfx_rect.bottom = rect.bottom();

  ctx->canvas->drawRect(tgfx_rect, paint);
}

void TgfxRenderer::DrawTextImpl(std::string_view text, const Point& position,
                                const Color& color, float font_size) {
  auto* ctx = static_cast<TgfxContext*>(tgfx_context_);
  if (ctx == nullptr || ctx->canvas == nullptr) {
    return;
  }

  tgfx::Paint paint;
  paint.setColor(tgfx::Color{
      static_cast<float>(color.r) / 255.0F,
      static_cast<float>(color.g) / 255.0F,
      static_cast<float>(color.b) / 255.0F,
      static_cast<float>(color.a) / 255.0F,
  });

  auto font = tgfx::Font::Make(nullptr, font_size);
  if (font != nullptr) {
    ctx->canvas->drawSimpleText(std::string(text), position.x, position.y,
                                *font, paint);
  }
}

void TgfxRenderer::SaveImpl() {
  auto* ctx = static_cast<TgfxContext*>(tgfx_context_);
  if (ctx != nullptr && ctx->canvas != nullptr) {
    ctx->canvas->save();
  }
}

void TgfxRenderer::RestoreImpl() {
  auto* ctx = static_cast<TgfxContext*>(tgfx_context_);
  if (ctx != nullptr && ctx->canvas != nullptr) {
    ctx->canvas->restore();
  }
}

void TgfxRenderer::TranslateImpl(float delta_x, float delta_y) {
  auto* ctx = static_cast<TgfxContext*>(tgfx_context_);
  if (ctx != nullptr && ctx->canvas != nullptr) {
    ctx->canvas->translate(delta_x, delta_y);
  }
}

void TgfxRenderer::ClipRectImpl(const Rect& rect) {
  auto* ctx = static_cast<TgfxContext*>(tgfx_context_);
  if (ctx == nullptr || ctx->canvas == nullptr) {
    return;
  }

  tgfx::Rect tgfx_rect;
  tgfx_rect.left = rect.x;
  tgfx_rect.top = rect.y;
  tgfx_rect.right = rect.right();
  tgfx_rect.bottom = rect.bottom();
  ctx->canvas->clipRect(tgfx_rect);
}

#else  // !NEOFLUX_TGFX_AVAILABLE

TgfxRenderer::TgfxRenderer() = default;

TgfxRenderer::~TgfxRenderer() = default;

bool TgfxRenderer::Init(int width, int height, void* /*native_handle*/) {
  width_ = width;
  height_ = height;
  initialized_ = true;
  LOG(WARNING) << "TgfxRenderer using stub implementation (tgfx not found)";
  return true;
}

void TgfxRenderer::BeginFrame(const Color& /*clear_color*/) {}

void TgfxRenderer::EndFrame() {}

void TgfxRenderer::Execute(const RenderCommand& /*command*/) {
  // Stub: no rendering when tgfx is not available.
}

void TgfxRenderer::Resize(int width, int height) {
  width_ = width;
  height_ = height;
}

int TgfxRenderer::GetWidth() const noexcept { return width_; }

int TgfxRenderer::GetHeight() const noexcept { return height_; }

void TgfxRenderer::DrawRectImpl(const Rect& /*rect*/, const Color& /*color*/) {}

void TgfxRenderer::DrawTextImpl(std::string_view /*text*/,
                                const Point& /*position*/,
                                const Color& /*color*/, float /*font_size*/) {}

void TgfxRenderer::SaveImpl() {}

void TgfxRenderer::RestoreImpl() {}

void TgfxRenderer::TranslateImpl(float /*delta_x*/, float /*delta_y*/) {}

void TgfxRenderer::ClipRectImpl(const Rect& /*rect*/) {}

#endif  // NEOFLUX_TGFX_AVAILABLE

}  // namespace neoflux
