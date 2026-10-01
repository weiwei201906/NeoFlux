// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - tgfx_renderer.cpp
//
// tgfx Canvas renderer. Replays RenderCommand objects as tgfx::Canvas draw
// calls. On desktop, GLFW owns the window and a WGL OpenGL context; this file
// hands that already-current context to tgfx's OpenGL (WGL) backend via
// tgfx::GLDevice::Current() and draws into the default framebuffer (id 0).
//
// All tgfx / GL / GLFW state lives in TgfxRenderer::Impl (Pimpl).
// =============================================================================

#include "neoflux/renderers/tgfx_renderer.h"

#include <string>
#include <utility>

#include <glog/logging.h>

#include "neoflux/core/font_manager.h"
#include "neoflux/core/types.h"

#if defined(NEOFLUX_PLATFORM_DESKTOP) && defined(NEOFLUX_HAVE_TGFX)
#include <GLFW/glfw3.h>

#include "tgfx/core/Canvas.h"
#include "tgfx/core/Color.h"
#include "tgfx/core/Font.h"
#include "tgfx/core/Paint.h"
#include "tgfx/core/RRect.h"
#include "tgfx/core/Rect.h"
#include "tgfx/core/Surface.h"
#include "tgfx/core/Typeface.h"
#include "tgfx/gpu/Backend.h"
#include "tgfx/gpu/Context.h"
#include "tgfx/gpu/opengl/GLDevice.h"
#include "tgfx/gpu/opengl/GLTypes.h"
#endif

namespace neoflux {

#if defined(NEOFLUX_PLATFORM_DESKTOP) && defined(NEOFLUX_HAVE_TGFX)
struct TgfxRenderer::Impl {
  GLFWwindow* window = nullptr;

  // tgfx objects. The device wraps the GLFW-owned WGL context; the context is
  // locked on the render thread for the whole frame.
  std::shared_ptr<tgfx::GLDevice> device;
  tgfx::Context* context = nullptr;
  std::shared_ptr<tgfx::Surface> surface;
  tgfx::Canvas* canvas = nullptr;

  std::shared_ptr<tgfx::Typeface> typeface;
  FontManager font_manager;

  int width = 0;    // Logical (window) size, layout coordinates.
  int height = 0;
  int fb_width = 0;  // Physical framebuffer size (for the render target).
  int fb_height = 0;
  bool ready = false;

  // Acquires the tgfx device/context for the already-current WGL context.
  // Must be called on the thread where the GLFW context is current.
  bool EnsureDevice() {
    if (ready) {
      return true;
    }
    device = tgfx::GLDevice::Current();
    if (device == nullptr) {
      LOG(ERROR) << "tgfx::GLDevice::Current() returned nullptr; no current "
                    "WGL context on this thread";
      return false;
    }
    context = device->lockContext();
    if (context == nullptr) {
      LOG(ERROR) << "tgfx device->lockContext() returned nullptr";
      return false;
    }
    ready = true;
    LOG(INFO) << "tgfx WGL device attached to existing GLFW context";
    return true;
  }
};
#else
struct TgfxRenderer::Impl {
  int width = 0;
  int height = 0;
};
#endif

TgfxRenderer::TgfxRenderer() : impl_(std::make_unique<Impl>()) {}

TgfxRenderer::~TgfxRenderer() = default;

bool TgfxRenderer::Init(int width, int height, void* native_handle) {
#if defined(NEOFLUX_PLATFORM_DESKTOP) && defined(NEOFLUX_HAVE_TGFX)
  impl_->window = static_cast<GLFWwindow*>(native_handle);
  impl_->width = width;
  impl_->height = height;
  impl_->font_manager.ScanDirectory("thirdparty/fonts");
  impl_->font_manager.ScanDirectory("../thirdparty/fonts");
  impl_->font_manager.ScanDirectory("../../thirdparty/fonts");
  const std::string default_font = impl_->font_manager.GetDefaultFont();
  if (!default_font.empty()) {
    const std::string path = impl_->font_manager.GetPath(default_font);
    if (!path.empty()) {
      impl_->typeface = tgfx::Typeface::MakeFromPath(path);
      if (impl_->typeface == nullptr) {
        LOG(WARNING) << "tgfx could not load typeface: " << path;
      }
    }
  } else {
    LOG(WARNING) << "No fonts found in thirdparty/fonts; text will be blank.";
  }
  return true;
#else
  (void)width;
  (void)height;
  (void)native_handle;
  return false;
#endif
}

void TgfxRenderer::BeginFrame(const Color& clear_color) {
#if defined(NEOFLUX_PLATFORM_DESKTOP) && defined(NEOFLUX_HAVE_TGFX)
  if (!impl_->EnsureDevice()) {
    return;
  }
  // Query the true framebuffer (physical) and window (logical) sizes so DPI
  // scaling is handled by a canvas scale: layout coordinates stay logical.
  int fb_w = 0;
  int fb_h = 0;
  glfwGetFramebufferSize(impl_->window, &fb_w, &fb_h);
  int win_w = impl_->width;
  int win_h = impl_->height;
  int queried_w = 0;
  int queried_h = 0;
  glfwGetWindowSize(impl_->window, &queried_w, &queried_h);
  if (queried_w > 0 && queried_h > 0) {
    win_w = queried_w;
    win_h = queried_h;
    impl_->width = win_w;
    impl_->height = win_h;
  }
  if (fb_w <= 0 || fb_h <= 0) {
    fb_w = win_w;
    fb_h = win_h;
  }

  // (Re)create the surface on the default framebuffer (id 0) whenever the
  // framebuffer size changes. Bottom-left origin matches GL; tgfx flips the
  // canvas internally so drawing uses y-down logical coordinates.
  if (impl_->surface == nullptr || fb_w != impl_->fb_width ||
      fb_h != impl_->fb_height) {
    impl_->fb_width = fb_w;
    impl_->fb_height = fb_h;
    tgfx::GLFrameBufferInfo frame_buffer;  // id=0 (default fb), GL_RGBA8.
    tgfx::BackendRenderTarget render_target(frame_buffer, fb_w, fb_h);
    impl_->surface = tgfx::Surface::MakeFrom(
        impl_->context, render_target, tgfx::ImageOrigin::BottomLeft);
    if (impl_->surface == nullptr) {
      LOG(ERROR) << "tgfx Surface::MakeFrom(default framebuffer) failed";
      return;
    }
  }

  impl_->canvas = impl_->surface->getCanvas();
  if (impl_->canvas == nullptr) {
    return;
  }
  // Map logical layout coordinates onto the physical framebuffer.
  const float sx = win_w > 0 ? static_cast<float>(fb_w) /
                                   static_cast<float>(win_w)
                             : 1.0F;
  const float sy = win_h > 0 ? static_cast<float>(fb_h) /
                                   static_cast<float>(win_h)
                             : 1.0F;
  impl_->canvas->save();
  impl_->canvas->scale(sx, sy);
  impl_->canvas->clear(
      tgfx::Color::FromRGBA(clear_color.r, clear_color.g, clear_color.b,
                            clear_color.a));
#else
  (void)clear_color;
#endif
}

void TgfxRenderer::EndFrame() {
#if defined(NEOFLUX_PLATFORM_DESKTOP) && defined(NEOFLUX_HAVE_TGFX)
  if (impl_->canvas != nullptr) {
    impl_->canvas->restore();
  }
  impl_->canvas = nullptr;
  // Submit recorded draws to the GL context; GLFW then swaps buffers.
  if (impl_->context != nullptr) {
    impl_->context->flushAndSubmit();
  }
  // Drop the surface so a resize is picked up next frame.
  impl_->surface.reset();
#endif
}

void TgfxRenderer::Execute(const RenderCommand& command) {
#if defined(NEOFLUX_PLATFORM_DESKTOP) && defined(NEOFLUX_HAVE_TGFX)
  if (impl_->canvas == nullptr) {
    return;
  }
  auto paint_for = [](const Color& c) {
    tgfx::Paint p;
    p.setColor(tgfx::Color::FromRGBA(c.r, c.g, c.b, c.a));
    return p;
  };
  switch (command.type) {
    case RenderCommandType::kDrawRect: {
      const auto rect = tgfx::Rect::MakeXYWH(command.rect.x, command.rect.y,
                                             command.rect.width,
                                             command.rect.height);
      impl_->canvas->drawRect(rect, paint_for(command.color));
      break;
    }
    case RenderCommandType::kDrawRoundedRect: {
      const auto rect = tgfx::Rect::MakeXYWH(command.rect.x, command.rect.y,
                                             command.rect.width,
                                             command.rect.height);
      const float radius = command.corner_radius;
      impl_->canvas->drawRRect(
          tgfx::RRect::MakeRectXY(rect, radius, radius),
          paint_for(command.color));
      break;
    }
    case RenderCommandType::kDrawText: {
      tgfx::Font font(impl_->typeface, command.font_size);
      impl_->canvas->drawSimpleText(command.text, command.point.x,
                                    command.point.y, font,
                                    paint_for(command.color));
      break;
    }
    case RenderCommandType::kSave:
      impl_->canvas->save();
      break;
    case RenderCommandType::kRestore:
      impl_->canvas->restore();
      break;
    case RenderCommandType::kTranslate:
      impl_->canvas->translate(command.translate_x, command.translate_y);
      break;
    case RenderCommandType::kClipRect:
      impl_->canvas->clipRect(tgfx::Rect::MakeXYWH(
          command.rect.x, command.rect.y, command.rect.width,
          command.rect.height));
      break;
    default:
      break;
  }
#else
  (void)command;
#endif
}

void TgfxRenderer::Resize(int width, int height) {
  impl_->width = width;
  impl_->height = height;
}

int TgfxRenderer::GetWidth() const noexcept { return impl_->width; }

int TgfxRenderer::GetHeight() const noexcept { return impl_->height; }

}  // namespace neoflux
