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
// Every platform goes through the same shape: a tgfx Window owns the GPU
// device, the graphics surface/swapchain, and presents on context->submit().
// Only the window CREATION is platform-specific:
//   - Desktop OpenGL:  Linux  -> tgfx::EGLWindow::MakeFrom(XID)   (X11)
//                      Win32 -> tgfx::WGLWindow::MakeFrom(HWND)
//                      Apple -> unsupported; build with TGFX_USE_METAL
//                              (tgfx's CGL path needs an ObjC++ shell).
//   - Desktop Vulkan / D3D12 / Metal: a backend tgfx Window
//     (VulkanWindow / D3D12Window / MetalWindow) wraps the GLFW-created
//     native window and owns the swapchain.
//   - Android: tgfx::EGLWindow::MakeFrom(ANativeWindow*).
//   - iOS: the EAGLWindow path requires the ObjC++ app shell (CAEAGLLayer);
//     not wired yet (explicit log, see EnsureDevice).
//
// GLFW is a pure window + input bridge on every platform: the window is
// created with GLFW_NO_API and NeoFlux never touches a GL/EGL/WGL context
// itself. All tgfx / GLFW state lives in TgfxRenderer::Impl (Pimpl).
//
// External frames (media playback) arrive as an opaque image id on the
// kDrawTexture command and are resolved to a CPU-backed tgfx::Image through
// FrameImageRegistry, then composited with Canvas::drawImageRect(). No GL
// interop, no GL types, and no GL header appears in this file.
// =============================================================================

#include "neoflux/renderers/tgfx_renderer.h"

#include <string>
#include <utility>

#include <glog/logging.h>

#include "neoflux/core/font_manager.h"
#include "neoflux/core/types.h"

#include "renderers/frame_image_registry.h"

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
#include "tgfx/gpu/Context.h"

#ifdef NEOFLUX_PLATFORM_DESKTOP

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// Expose the GLFW native-accessors so the tgfx Window can be built from the
// platform's native window handle (XID on X11, HWND on Windows, NSWindow* on
// Apple). The matching window-system headers come via glfw3native.h.
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>

#include "tgfx/gpu/Window.h"

#ifdef TGFX_USE_OPENGL
#ifdef _WIN32
#include "tgfx/gpu/opengl/wgl/WGLWindow.h"
#elif defined(__linux__)
#include "tgfx/gpu/opengl/egl/EGLWindow.h"
#elif defined(__APPLE__)
#error "NeoFlux on Apple requires the tgfx Metal backend (-DTGFX_USE_METAL=ON); \
the tgfx OpenGL backend needs an ObjC++ (CGL) app shell we do not provide."
#endif
#elif defined(TGFX_USE_VULKAN)
#include "tgfx/gpu/vulkan/VulkanDevice.h"
#include "tgfx/gpu/vulkan/VulkanWindow.h"
#elif defined(TGFX_USE_D3D12)
#include "tgfx/gpu/d3d12/D3D12Device.h"
#include "tgfx/gpu/d3d12/D3D12Window.h"
#elif defined(TGFX_USE_METAL)
#include "tgfx/gpu/metal/MetalDevice.h"
#include "tgfx/gpu/metal/MetalWindow.h"
#endif  // TGFX_USE_OPENGL / VULKAN / D3D12 / METAL

#else  // mobile

#ifdef __ANDROID__
#include <android/native_window.h>
#include "tgfx/gpu/opengl/egl/EGLWindow.h"
#endif

#endif  // NEOFLUX_PLATFORM_DESKTOP

namespace neoflux {

struct TgfxRenderer::Impl {
  // The native window handed over by the platform layer: GLFWwindow* on
  // desktop, ANativeWindow* on Android (CAEAGLLayer* on iOS once the ObjC++
  // shell exists). Assigned in Init(); the tgfx Window is built from it
  // lazily in EnsureDevice() on the render thread.
  void* native_window = nullptr;

  // tgfx objects. The Device comes from the tgfx Window; the context is
  // locked on the render thread for the whole frame.
  std::shared_ptr<tgfx::Device> device;
  tgfx::Context* context = nullptr;
  std::shared_ptr<tgfx::Surface> surface;
  tgfx::Canvas* canvas = nullptr;

  // The tgfx Window owns the graphics surface/swapchain on every platform
  // and presents automatically on context->submit().
  std::shared_ptr<tgfx::Window> tgfx_window;

  std::shared_ptr<tgfx::Typeface> typeface;
  FontManager font_manager;

  int width = 0;  // Logical (window) size, layout coordinates.
  int height = 0;
  bool ready = false;

  // Builds the tgfx Window for the platform's native window and locks the
  // render context. Called on the render thread from BeginFrame().
  bool EnsureDevice() {
    if (ready) {
      return true;
    }
#ifdef NEOFLUX_PLATFORM_DESKTOP
#ifdef TGFX_USE_OPENGL
    // Desktop OpenGL: tgfx creates and owns the EGL/WGL context and surface
    // for the native window; NeoFlux holds no GL context of its own.
#ifdef _WIN32
    auto* const hwnd = glfwGetWin32Window(static_cast<GLFWwindow*>(native_window));
    if (hwnd == nullptr) {
      LOG(ERROR) << "glfwGetWin32Window() returned nullptr";
      return false;
    }
    tgfx_window = tgfx::WGLWindow::MakeFrom(hwnd);
    if (tgfx_window == nullptr) {
      LOG(ERROR) << "tgfx::WGLWindow::MakeFrom(HWND) failed";
      return false;
    }
#elif defined(__linux__)
    // X11: the EGL native window is the XID (Window) of the GLFW window.
    const auto xid = glfwGetX11Window(static_cast<GLFWwindow*>(native_window));
    if (xid == 0) {
      LOG(ERROR) << "glfwGetX11Window() returned 0 (is GLFW using the X11 "
                    "backend?)";
      return false;
    }
    tgfx_window = tgfx::EGLWindow::MakeFrom(xid);
    if (tgfx_window == nullptr) {
      LOG(ERROR) << "tgfx::EGLWindow::MakeFrom(XID) failed";
      return false;
    }
#else
    LOG(ERROR) << "tgfx OpenGL backend is not supported on this desktop "
                  "platform (Apple requires -DTGFX_USE_METAL=ON)";
    return false;
#endif
    device = tgfx_window->getDevice();
    LOG(INFO) << "tgfx GL window surface ready (" << width << "x" << height
              << ")";
#elif defined(TGFX_USE_VULKAN)
    // VulkanWindow only exposes a Win32 (HWND) target on Windows in the pinned
    // tgfx revision; see the header for the Android/OHOS overloads.
#ifdef _WIN32
    auto vk_device = tgfx::VulkanDevice::Make();
    if (vk_device == nullptr) {
      LOG(ERROR) << "tgfx::VulkanDevice::Make() failed: no usable Vulkan device";
      return false;
    }
    device = vk_device;
    tgfx_window = tgfx::VulkanWindow::MakeFrom(glfwGetWin32Window(
        static_cast<GLFWwindow*>(native_window)), vk_device);
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
    tgfx_window = tgfx::D3D12Window::MakeForHwnd(
        glfwGetWin32Window(static_cast<GLFWwindow*>(native_window)),
        d3d_device);
#elif defined(TGFX_USE_METAL)
    auto mtl_device = tgfx::MetalDevice::Make();
    if (mtl_device == nullptr) {
      LOG(ERROR) << "tgfx::MetalDevice::Make() failed: no usable Metal device";
      return false;
    }
    device = mtl_device;
    // GLFW owns the NSWindow when it was created with GLFW_NO_API; attach a
    // CAMetalLayer to its content view and let tgfx present into it.
    NSWindow* native_window = glfwGetCocoaWindow(
        static_cast<GLFWwindow*>(this->native_window));
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
    LOG(ERROR) << "No tgfx GPU backend enabled in this build";
    return false;
#endif  // TGFX_USE_OPENGL / VULKAN / D3D12 / METAL
#else   // mobile
#ifdef __ANDROID__
    auto* const anw = static_cast<ANativeWindow*>(native_window);
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
    LOG(INFO) << "tgfx EGLWindow surface ready (" << width << "x" << height
              << ")";
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
#endif  // NEOFLUX_PLATFORM_DESKTOP

    if (device == nullptr) {
      device = tgfx_window->getDevice();
    }
    context = device->lockContext();
    if (context == nullptr) {
      LOG(ERROR) << "tgfx device->lockContext() returned nullptr";
      return false;
    }
    ready = true;
    return true;
  }
};

TgfxRenderer::TgfxRenderer() : impl_(std::make_unique<Impl>()) {}

TgfxRenderer::~TgfxRenderer() = default;

bool TgfxRenderer::Init(int width, int height, void* native_handle) {
  impl_->width = width;
  impl_->height = height;
  // Desktop: the GLFWwindow* (created with GLFW_NO_API). Mobile: the native
  // window (ANativeWindow* on Android). The tgfx Window (and through it the
  // device/context/surface) is created lazily in EnsureDevice() on the
  // render thread.
  if (native_handle == nullptr) {
    LOG(ERROR) << "TgfxRenderer::Init: null native window from the platform "
                  "layer";
    return false;
  }
  impl_->native_window = native_handle;

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
}

void TgfxRenderer::BeginFrame(const Color& clear_color) {
  if (!impl_->EnsureDevice()) {
    return;
  }
  // Query the true framebuffer (physical) and window (logical) sizes so DPI
  // scaling is handled by a canvas scale: layout coordinates stay logical.
  int fb_w = 0;
  int fb_h = 0;
  int win_w = impl_->width;
  int win_h = impl_->height;
#ifdef NEOFLUX_PLATFORM_DESKTOP
  auto* const window = static_cast<GLFWwindow*>(impl_->native_window);
  glfwGetFramebufferSize(window, &fb_w, &fb_h);
  int queried_w = 0;
  int queried_h = 0;
  glfwGetWindowSize(window, &queried_w, &queried_h);
  if (queried_w > 0 && queried_h > 0) {
    win_w = queried_w;
    win_h = queried_h;
    impl_->width = win_w;
    impl_->height = win_h;
  }
#elif defined(__ANDROID__)
  auto* const anw = static_cast<ANativeWindow*>(impl_->native_window);
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

  // The tgfx Window owns the render target (its swapchain backbuffer) and
  // picks up size changes itself, so a surface is acquired per frame rather
  // than kept across resizes.
  impl_->surface = tgfx::Surface::MakeFrom(impl_->context, impl_->tgfx_window);
  if (impl_->surface == nullptr) {
    LOG(ERROR) << "tgfx Surface::MakeFrom(context, window) failed";
    return;
  }

  impl_->canvas = impl_->surface->getCanvas();
  if (impl_->canvas == nullptr) {
    return;
  }
  impl_->canvas->save();
  impl_->canvas->scale(sx, sy);
  impl_->canvas->clear(
      tgfx::Color::FromRGBA(clear_color.r, clear_color.g, clear_color.b,
                            clear_color.a));
}

void TgfxRenderer::EndFrame() {
  if (impl_->canvas != nullptr) {
    impl_->canvas->restore();
  }
  impl_->canvas = nullptr;
  // flush returns a Recording that is submitted to the GPU; submitting on a
  // Window-backed surface also presents it (swapchain flip / eglSwapBuffers /
  // wglSwapBuffers), so no platform-side buffer swap exists anywhere.
  if (impl_->context != nullptr) {
    auto recording = impl_->context->flush();
    if (recording != nullptr) {
      impl_->context->submit(std::move(recording));
    }
  }
  // Drop the surface so a resize is picked up next frame.
  impl_->surface.reset();
}

void TgfxRenderer::Execute(const RenderCommand& command) {
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
    case RenderCommandType::kDrawTexture: {
      // External frame compositing. The command carries only an opaque image id;
      // the producer (the media module) registered a CPU-backed tgfx::Image
      // under that id and keeps it valid until it releases it. Resolving the id
      // here is backend-agnostic: drawImageRect() uploads the CPU frame through
      // whichever tgfx backend is active (OpenGL, Metal, Vulkan, D3D12), so no
      // GL interop and no GL header is involved anywhere in NeoFlux.
      auto image = FrameImageRegistry::Find(command.image_id);
      if (image == nullptr) {
        // Unknown or already released id (the producer tore its frame down
        // first, or the command outlived the frame): draw nothing.
        break;
      }
      const auto dest = tgfx::Rect::MakeXYWH(command.rect.x, command.rect.y,
                                             command.rect.width,
                                             command.rect.height);
      impl_->canvas->drawImageRect(image, dest, tgfx::SamplingOptions());
      break;
    }
    default:
      break;
  }
}

void TgfxRenderer::Resize(int width, int height) {
  impl_->width = width;
  impl_->height = height;
}

int TgfxRenderer::GetWidth() const noexcept { return impl_->width; }

int TgfxRenderer::GetHeight() const noexcept { return impl_->height; }

}  // namespace neoflux
