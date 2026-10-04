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
#include "tgfx/core/Image.h"
#include "tgfx/core/Paint.h"
#include "tgfx/core/RRect.h"
#include "tgfx/core/Rect.h"
#include "tgfx/core/SamplingOptions.h"
#include "tgfx/core/Surface.h"
#include "tgfx/core/Typeface.h"
#include "tgfx/gpu/Backend.h"
#include "tgfx/gpu/Context.h"
#include "tgfx/gpu/opengl/GLDevice.h"
#include "tgfx/gpu/opengl/GLTypes.h"

#if !defined(NEOFLUX_BACKEND_gl)
// Non-OpenGL backends render through the tgfx Window abstraction instead of the
// GL device: a backend Window owns the swapchain and presents on submit().
// See the #else of TgfxRenderer::Impl::EnsureDevice() for how it is acquired.
#include <GLFW/glfw3native.h>

#include "tgfx/gpu/Window.h"
#if defined(NEOFLUX_BACKEND_vulkan)
#include "tgfx/gpu/vulkan/VulkanDevice.h"
#include "tgfx/gpu/vulkan/VulkanWindow.h"
#elif defined(NEOFLUX_BACKEND_d3d12)
#include "tgfx/gpu/d3d12/D3D12Device.h"
#include "tgfx/gpu/d3d12/D3D12Window.h"
#elif defined(NEOFLUX_BACKEND_metal)
#include "tgfx/gpu/metal/MetalDevice.h"
#include "tgfx/gpu/metal/MetalWindow.h"
#endif
#endif
#endif

namespace neoflux {

#if defined(NEOFLUX_PLATFORM_DESKTOP) && defined(NEOFLUX_HAVE_TGFX)
struct TgfxRenderer::Impl {
  // The GLFW window whose native render surface/context backs `device`. Assigned
  // in Init() from the native handle passed by RenderLayer::Start().
  GLFWwindow* window = nullptr;

  // tgfx objects. The device wraps the window's native rendering context; the
  // context is locked on the render thread for the whole frame.
  std::shared_ptr<tgfx::Device> device;
  tgfx::Context* context = nullptr;
  std::shared_ptr<tgfx::Surface> surface;
  tgfx::Canvas* canvas = nullptr;

  // Non-OpenGL backends: the tgfx Window owns the graphics surface/swapchain and
  // presents automatically on context->submit(). Unused on the OpenGL path,
  // which renders into the default framebuffer that GLFW swaps.
#if !defined(NEOFLUX_BACKEND_gl)
  std::shared_ptr<tgfx::Window> tgfx_window;
#endif

  std::shared_ptr<tgfx::Typeface> typeface;
  FontManager font_manager;

  int width = 0;    // Logical (window) size, layout coordinates.
  int height = 0;
  int fb_width = 0;  // Physical framebuffer size (for the render target).
  int fb_height = 0;
  bool ready = false;

  // Acquires the tgfx device/context for the window's native rendering context.
  // Must be called on the thread where that context is current (OpenGL) or will
  // be used (other backends).
#if defined(NEOFLUX_BACKEND_gl)
  bool EnsureDevice() {
    if (ready) {
      return true;
    }
    // Attach to the WGL context GLFW already made current on this thread.
    auto gl_device = tgfx::GLDevice::Current();
    if (gl_device == nullptr) {
      LOG(ERROR) << "tgfx::GLDevice::Current() returned nullptr; no current "
                    "WGL context on this thread";
      return false;
    }
    device = gl_device;
    context = device->lockContext();
    if (context == nullptr) {
      LOG(ERROR) << "tgfx device->lockContext() returned nullptr";
      return false;
    }
    ready = true;
    LOG(INFO) << "tgfx WGL device attached to existing GLFW context";
    return true;
  }
#else
  // Creates the backend device and wraps the GLFW native window in a tgfx
  // Window. Each backend needs a different native handle type. Only the device
  // creation below is backend-specific; the per-frame drawing code is shared
  // through the tgfx Window/Surface abstraction.
  bool EnsureDevice() {
    if (ready) {
      return true;
    }
#if defined(NEOFLUX_BACKEND_vulkan)
    // VulkanWindow only exposes a Win32 (HWND) target on Windows in the pinned
    // tgfx revision; see the header for the Android/OHOS overloads.
#if defined(_WIN32)
    auto vk_device = tgfx::VulkanDevice::Make();
    if (vk_device == nullptr) {
      LOG(ERROR) << "tgfx::VulkanDevice::Make() failed: no usable Vulkan device";
      return false;
    }
    device = vk_device;
    tgfx_window = tgfx::VulkanWindow::MakeFrom(glfwGetWin32Window(window), vk_device);
#else
    LOG(ERROR) << "NEOFLUX_BACKEND=vulkan has no window binding for this "
                  "platform in this tgfx revision (only Win32/Android/OHOS)";
    return false;
#endif  // _WIN32

#elif defined(NEOFLUX_BACKEND_d3d12)
    auto d3d_device = tgfx::D3D12Device::Make();
    if (d3d_device == nullptr) {
      LOG(ERROR) << "tgfx::D3D12Device::Make() failed: no usable D3D12 device";
      return false;
    }
    device = d3d_device;
    tgfx_window = tgfx::D3D12Window::MakeForHwnd(glfwGetWin32Window(window), d3d_device);

#elif defined(NEOFLUX_BACKEND_metal)
    auto mtl_device = tgfx::MetalDevice::Make();
    if (mtl_device == nullptr) {
      LOG(ERROR) << "tgfx::MetalDevice::Make() failed: no usable Metal device";
      return false;
    }
    device = mtl_device;
    // GLFW owns the NSWindow when it was created with GLFW_NO_API; attach a
    // CAMetalLayer to its content view and let tgfx present into it.
    NSWindow* native_window = glfwGetCocoaWindow(window);
    if (native_window == nil) {
      LOG(ERROR) << "glfwGetCocoaWindow() returned nil";
      return false;
    }
    CAMetalLayer* layer = [CAMetalLayer layer];
    layer.device = mtl_device->metalDevice();
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    layer.framebufferOnly = YES;
    native_window.contentView.layer = layer;
    native_window.contentView.wantsLayer = YES;
    tgfx_window = tgfx::MetalWindow::MakeFrom(layer, mtl_device);
#else
#error "Unknown NEOFLUX_BACKEND: expected gl, vulkan, d3d12 or metal."
#endif

    if (tgfx_window == nullptr) {
      LOG(ERROR) << "Failed to create the tgfx Window for backend="
                 << NEOFLUX_BACKEND_NAME;
      return false;
    }
    // A Window's device may still be initializing; getDevice() can return null
    // briefly, but by construction the device we passed in is alive.
    context = device->lockContext();
    if (context == nullptr) {
      LOG(ERROR) << "tgfx device->lockContext() returned nullptr";
      return false;
    }
    ready = true;
    LOG(INFO) << "tgfx " << NEOFLUX_BACKEND_NAME << " device and window ready";
    return true;
  }
#endif
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
  // Desktop passes the GLFWwindow*; the backend device is created lazily in
  // BeginFrame() on the render thread, where the native context is valid.
  impl_->window = static_cast<GLFWwindow*>(native_handle);
  impl_->width = width;
  impl_->height = height;
  // Fonts live in the project's assets/fonts/ (not thirdparty/). Probe the
  // working-directory relative locations the binary can be launched from.
  impl_->font_manager.ScanDirectory("assets/fonts");
  impl_->font_manager.ScanDirectory("../assets/fonts");
  impl_->font_manager.ScanDirectory("../../assets/fonts");
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

  // Map logical layout coordinates onto the physical framebuffer.
  const float sx = win_w > 0 ? static_cast<float>(fb_w) /
                                   static_cast<float>(win_w)
                             : 1.0F;
  const float sy = win_h > 0 ? static_cast<float>(fb_h) /
                                   static_cast<float>(win_h)
                             : 1.0F;
#if defined(NEOFLUX_BACKEND_gl)
  // (Re)create the surface on the GL default framebuffer (id 0) whenever the
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
#else
  // Non-OpenGL backends: the tgfx Window owns the render target (its swapchain
  // backbuffer) and picks up size changes itself, so a surface is acquired per
  // frame rather than kept across resizes. Window surfaces use a top-left
  // origin, unlike the GL default framebuffer.
  impl_->surface = tgfx::Surface::MakeFrom(impl_->context, impl_->tgfx_window);
  if (impl_->surface == nullptr) {
    LOG(ERROR) << "tgfx Surface::MakeFrom(context, window) failed";
    return;
  }
#endif

  impl_->canvas = impl_->surface->getCanvas();
  if (impl_->canvas == nullptr) {
    return;
  }
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
#if defined(NEOFLUX_BACKEND_gl)
  // Submit recorded draws to the GL context; GLFW then swaps buffers.
  if (impl_->context != nullptr) {
    impl_->context->flushAndSubmit();
  }
#else
  // Submitting on a Window surface also presents it (swapchain flip); GLFW does
  // NOT swap buffers for these backends.
  if (impl_->context != nullptr) {
    impl_->context->submit();
  }
#endif
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
#if defined(NEOFLUX_BACKEND_gl)
    case RenderCommandType::kDrawTexture: {
      // Media module (GL backend only): wrap an externally-produced GL texture
      // (mpv render context) as a tgfx BackendTexture and draw it into the
      // destination rect. MakeFrom does NOT take ownership of the GL texture;
      // the producer (mpv) manages its lifetime. The texture id stays the same
      // across frames; only its contents are updated by mpv, so re-creating
      // the Image each frame is cheap (just a handle, no GPU upload).
      // On other backends this case does not exist at all: the media module is
      // bound to the GL backend (see neoflux/CMakeLists.txt) and MediaWidget
      // degrades to a placeholder, so no kDrawTexture command is ever sent.
      tgfx::GLTextureInfo gl_info{};
      gl_info.id = command.texture_id;
      gl_info.target = 0x0DE1U;   // GL_TEXTURE_2D
      gl_info.format = 0x8058U;   // GL_RGBA8
      const auto src_w = static_cast<int>(command.rect.width);
      const auto src_h = static_cast<int>(command.rect.height);
      tgfx::BackendTexture backend(gl_info, src_w, src_h);
      auto image = tgfx::Image::MakeFrom(impl_->context, backend);
      if (image != nullptr) {
        auto dest = tgfx::Rect::MakeXYWH(command.rect.x, command.rect.y,
                                          command.rect.width,
                                          command.rect.height);
        impl_->canvas->drawImageRect(image, dest,
                                     tgfx::SamplingOptions());
      }
      break;
    }
#endif
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
