// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - tgfx_renderer.cpp
//
// tgfx Canvas renderer. Replays RenderCommand objects as tgfx::Canvas draw
// calls. ALL GPU-backend knowledge lives in tgfx: this file only consumes
// tgfx's Device / Window / Surface abstractions and has no backend branching
// of its own (backend selection is tgfx's TGFX_USE_* compile-time choice, see
// thirdparty/CMakeLists.txt).
//
// Surface acquisition by platform:
//   - Desktop OpenGL: GLFW owns the window and a WGL/GLX context; this file
//     hands that already-current context to tgfx via tgfx::GLDevice::Current()
//     and draws into the default framebuffer (id 0). GLFW swaps buffers.
//   - Desktop Vulkan / D3D12 / Metal: a tgfx Window (VulkanWindow /
//     D3D12Window / MetalWindow) owns the swapchain and presents on submit().
//   - Android: tgfx::EGLWindow creates and owns the EGL display/context/
//     surface for the ANativeWindow handed over by the app shell, and presents
//     on submit().
//   - iOS: the EAGLWindow path requires the ObjC++ app shell (CAEAGLLayer);
//     not wired yet (explicit log, see EnsureDevice).
//
// All tgfx / GL / GLFW state lives in TgfxRenderer::Impl (Pimpl).
// =============================================================================

#include "neoflux/renderers/tgfx_renderer.h"

#include <string>
#include <utility>

#include <glog/logging.h>

#include "neoflux/core/font_manager.h"
#include "neoflux/core/types.h"

#if defined(NEOFLUX_HAVE_TGFX)
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

#if defined(NEOFLUX_PLATFORM_DESKTOP)
#include <GLFW/glfw3.h>
#if defined(TGFX_USE_OPENGL)
// Desktop OpenGL: attach to the context GLFW made current (GLDevice.h).
#include "tgfx/gpu/opengl/GLDevice.h"
#include "tgfx/gpu/opengl/GLTypes.h"
#else
// Desktop Vulkan / D3D12 / Metal: render through the tgfx Window abstraction —
// a backend Window owns the swapchain and presents on submit().
#include <GLFW/glfw3native.h>

#include "tgfx/gpu/Window.h"
#if defined(TGFX_USE_VULKAN)
#include "tgfx/gpu/vulkan/VulkanDevice.h"
#include "tgfx/gpu/vulkan/VulkanWindow.h"
#elif defined(TGFX_USE_D3D12)
#include "tgfx/gpu/d3d12/D3D12Device.h"
#include "tgfx/gpu/d3d12/D3D12Window.h"
#elif defined(TGFX_USE_METAL)
#include "tgfx/gpu/metal/MetalDevice.h"
#include "tgfx/gpu/metal/MetalWindow.h"
#endif
#endif  // TGFX_USE_OPENGL
#else
// Mobile: tgfx owns the EGL/EAGL context and swapchain through its Window
// abstraction; the platform only hands us the native window handle.
#if defined(__ANDROID__)
#include <android/native_window.h>
#include "tgfx/gpu/opengl/egl/EGLWindow.h"
#endif
#endif  // NEOFLUX_PLATFORM_DESKTOP
#endif  // NEOFLUX_HAVE_TGFX

namespace neoflux {

#if defined(NEOFLUX_HAVE_TGFX)
struct TgfxRenderer::Impl {
#if defined(NEOFLUX_PLATFORM_DESKTOP)
  // The GLFW window whose native render surface/context backs `device`.
  // Assigned in Init() from the native handle passed by RenderLayer::Start().
  GLFWwindow* window = nullptr;
#else
  // Mobile: the native window handle from the app shell (ANativeWindow* on
  // Android, CAEAGLLayer* on iOS once the ObjC++ shell exists).
  void* native_window = nullptr;
#endif

  // tgfx objects. The device wraps the window's native rendering context; the
  // context is locked on the render thread for the whole frame.
  std::shared_ptr<tgfx::Device> device;
  tgfx::Context* context = nullptr;
  std::shared_ptr<tgfx::Surface> surface;
  tgfx::Canvas* canvas = nullptr;

  // Window-based paths — desktop Vulkan/D3D12/Metal AND every mobile build:
  // the tgfx Window owns the graphics surface/swapchain and presents
  // automatically on context->submit(). Unused on the desktop-GL path, which
  // renders into the default framebuffer that GLFW swaps.
#if !defined(NEOFLUX_PLATFORM_DESKTOP) || !defined(TGFX_USE_OPENGL)
  std::shared_ptr<tgfx::Window> tgfx_window;
#endif

  std::shared_ptr<tgfx::Typeface> typeface;
  FontManager font_manager;

  int width = 0;    // Logical (window) size, layout coordinates.
  int height = 0;
  int fb_width = 0;  // Physical framebuffer size (for the render target).
  int fb_height = 0;
  bool ready = false;

  // Acquires the tgfx device/context for the window's native rendering
  // surface. Must be called on the thread where that context is current
  // (desktop OpenGL) or where rendering will happen (all other paths).
  bool EnsureDevice() {
    if (ready) {
      return true;
    }
#if defined(NEOFLUX_PLATFORM_DESKTOP) && defined(TGFX_USE_OPENGL)
    // Attach to the WGL/GLX context GLFW already made current on this thread.
    auto gl_device = tgfx::GLDevice::Current();
    if (gl_device == nullptr) {
      LOG(ERROR) << "tgfx::GLDevice::Current() returned nullptr; no current "
                    "GL context on this thread";
      return false;
    }
    device = gl_device;
    LOG(INFO) << "tgfx GL device attached to existing GLFW context";
#elif defined(NEOFLUX_PLATFORM_DESKTOP)
    // Desktop Vulkan / D3D12 / Metal: create the backend device and wrap the
    // GLFW native window in a tgfx Window. Only device/window creation is
    // backend-specific; the per-frame drawing code is shared through the
    // tgfx Window/Surface abstraction.
#if defined(TGFX_USE_VULKAN)
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
    LOG(ERROR) << "tgfx's Vulkan backend has no window binding for this "
                  "platform in this revision (only Win32/Android/OHOS)";
    return false;
#endif  // _WIN32
#elif defined(TGFX_USE_D3D12)
    auto d3d_device = tgfx::D3D12Device::Make();
    if (d3d_device == nullptr) {
      LOG(ERROR) << "tgfx::D3D12Device::Make() failed: no usable D3D12 device";
      return false;
    }
    device = d3d_device;
    tgfx_window = tgfx::D3D12Window::MakeForHwnd(glfwGetWin32Window(window), d3d_device);
#elif defined(TGFX_USE_METAL)
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
#endif  // backend selection
    if (tgfx_window == nullptr) {
      LOG(ERROR) << "Failed to create the tgfx Window for the selected backend";
      return false;
    }
#else
  // Mobile: tgfx creates and owns the EGL display/context/surface for the
  // ANativeWindow and presents (eglSwapBuffers) on context->submit().
#if defined(__ANDROID__)
    auto* anw = static_cast<ANativeWindow*>(native_window);
    if (anw == nullptr) {
      LOG(ERROR) << "EnsureDevice: null ANativeWindow from the app shell";
      return false;
    }
    tgfx_window = tgfx::EGLWindow::MakeFrom(anw);
    if (tgfx_window == nullptr) {
      LOG(ERROR) << "tgfx::EGLWindow::MakeFrom(ANativeWindow) failed";
      return false;
    }
    device = tgfx_window->getDevice();
    LOG(INFO) << "tgfx EGLWindow surface ready (" << width << "x" << height << ")";
#elif defined(__APPLE__) && defined(TARGET_OS_IPHONE)
    // iOS: EAGLWindow::MakeFrom(CAEAGLLayer*) is the equivalent path, but the
    // layer is obtained from the ObjC++ app shell (UIView hierarchy), which
    // does not exist in this repository yet. Fail explicitly, never silently.
    LOG(ERROR) << "iOS rendering shell not wired yet: EAGLWindow::MakeFrom("
                  "CAEAGLLayer*) must be called from the ObjC++ app shell";
    return false;
#else
    LOG(ERROR) << "No tgfx Window binding for this mobile platform";
    return false;
#endif  // __ANDROID__ / iOS
#endif  // desktop-GL / desktop-other / mobile

    context = device->lockContext();
    if (context == nullptr) {
      LOG(ERROR) << "tgfx device->lockContext() returned nullptr";
      return false;
    }
    ready = true;
    return true;
  }
};
#else
struct TgfxRenderer::Impl {
  int width = 0;
  int height = 0;
};
#endif  // NEOFLUX_HAVE_TGFX

TgfxRenderer::TgfxRenderer() : impl_(std::make_unique<Impl>()) {}

TgfxRenderer::~TgfxRenderer() = default;

bool TgfxRenderer::Init(int width, int height, void* native_handle) {
#if defined(NEOFLUX_HAVE_TGFX)
  impl_->width = width;
  impl_->height = height;
#if defined(NEOFLUX_PLATFORM_DESKTOP)
  // Desktop passes the GLFWwindow*; the backend device is created lazily in
  // BeginFrame() on the render thread, where the native context is valid.
  impl_->window = static_cast<GLFWwindow*>(native_handle);
#else
  // Mobile: the app shell hands over the native window (ANativeWindow* on
  // Android). tgfx::EGLWindow creates and owns the EGL context/surface from
  // it lazily in EnsureDevice() on the render thread.
  if (native_handle == nullptr) {
    LOG(ERROR) << "TgfxRenderer::Init: null native window from the app shell";
    return false;
  }
  impl_->native_window = native_handle;
#endif
  // Fonts live in the project's assets/fonts/ (not thirdparty/). Probe the
  // working-directory relative locations the binary can be launched from; on
  // mobile the app shell is responsible for extracting bundled assets to one
  // of these paths (best-effort: text renders blank with a warning otherwise).
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
  // tgfx is not compiled into this build (NEOFLUX_HAVE_TGFX undefined).
  LOG(ERROR) << "TgfxRenderer::Init: tgfx is not available in this build";
  (void)width;
  (void)height;
  (void)native_handle;
  return false;
#endif
}

void TgfxRenderer::BeginFrame(const Color& clear_color) {
#if defined(NEOFLUX_HAVE_TGFX)
  if (!impl_->EnsureDevice()) {
    return;
  }
  // Query the true framebuffer (physical) and window (logical) sizes so DPI
  // scaling is handled by a canvas scale: layout coordinates stay logical.
  int fb_w = 0;
  int fb_h = 0;
  int win_w = impl_->width;
  int win_h = impl_->height;
#if defined(NEOFLUX_PLATFORM_DESKTOP)
  glfwGetFramebufferSize(impl_->window, &fb_w, &fb_h);
  int queried_w = 0;
  int queried_h = 0;
  glfwGetWindowSize(impl_->window, &queried_w, &queried_h);
  if (queried_w > 0 && queried_h > 0) {
    win_w = queried_w;
    win_h = queried_h;
    impl_->width = win_w;
    impl_->height = win_h;
  }
#elif defined(__ANDROID__)
  auto* anw = static_cast<ANativeWindow*>(impl_->native_window);
  if (anw != nullptr) {
    fb_w = ANativeWindow_getWidth(anw);
    fb_h = ANativeWindow_getHeight(anw);
    // The app shell supplies layout coordinates in surface pixels for now;
    // density-aware scaling belongs to the shell integration.
    win_w = fb_w;
    win_h = fb_h;
    impl_->width = win_w;
    impl_->height = win_h;
  }
#endif
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
#if defined(NEOFLUX_PLATFORM_DESKTOP) && defined(TGFX_USE_OPENGL)
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
  // tgfx Window paths (desktop Vulkan/D3D12/Metal and every mobile build):
  // the Window owns the render target (its swapchain backbuffer) and picks up
  // size changes itself, so a surface is acquired per frame rather than kept
  // across resizes. Window surfaces use a top-left origin, unlike the GL
  // default framebuffer.
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
#if defined(NEOFLUX_HAVE_TGFX)
  if (impl_->canvas != nullptr) {
    impl_->canvas->restore();
  }
  impl_->canvas = nullptr;
#if defined(NEOFLUX_PLATFORM_DESKTOP) && defined(TGFX_USE_OPENGL)
  // Submit recorded draws to the GL context; GLFW then swaps buffers.
  if (impl_->context != nullptr) {
    impl_->context->flushAndSubmit();
  }
#else
  // Window paths (desktop Vulkan/D3D12/Metal and every mobile build):
  // flush returns a Recording that is submitted to the GPU; submitting on a
  // Window-backed surface also presents it (swapchain flip / eglSwapBuffers),
  // so GLFW does NOT swap buffers for these paths.
  if (impl_->context != nullptr) {
    auto recording = impl_->context->flush();
    if (recording != nullptr) {
      impl_->context->submit(std::move(recording));
    }
  }
#endif
  // Drop the surface so a resize is picked up next frame.
  impl_->surface.reset();
#endif
}

void TgfxRenderer::Execute(const RenderCommand& command) {
#if defined(NEOFLUX_HAVE_TGFX)
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
#if defined(TGFX_USE_OPENGL)
    case RenderCommandType::kDrawTexture: {
      // Media module (GL backend only): wrap an externally-produced GL texture
      // (mpv render context) as a tgfx BackendTexture and draw it into the
      // destination rect. MakeFrom does NOT take ownership of the GL texture;
      // the producer (mpv) manages its lifetime. The texture id stays the same
      // across frames; only its contents are updated by mpv, so re-creating
      // the Image each frame is cheap (just a handle, no GPU upload).
      // On non-OpenGL tgfx builds this case does not exist at all: the media
      // module is bound to OpenGL (see neoflux/CMakeLists.txt) and MediaWidget
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
#endif
}

void TgfxRenderer::Resize(int width, int height) {
  impl_->width = width;
  impl_->height = height;
}

int TgfxRenderer::GetWidth() const noexcept { return impl_->width; }

int TgfxRenderer::GetHeight() const noexcept { return impl_->height; }

}  // namespace neoflux
