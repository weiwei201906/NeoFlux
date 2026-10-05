// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - Mobile Platform Bridge
//
// Platform bridge implementation for mobile (Android / iOS). Unlike the
// desktop GLFW bridge, mobile does not create its own window; the platform
// provides a native surface (ANativeWindow on Android, CAEAGLLayer on iOS)
// that the renderer draws into.
//
// OWNERSHIP (important): the rendering context and swapchain belong to tgfx:
// TgfxRenderer wraps the native surface in a tgfx::EGLWindow (Android) /
// EAGLWindow (iOS), which creates the EGL/EAGL display, context and surface
// and presents on context->submit(). This bridge deliberately does NOT create
// any EGL/EAGL objects; it only:
//   - carries the native window handle for TgfxRenderer::Init(),
//   - dispatches touch events from the platform shell into the widget tree
//     via DispatchTouchEvent() (called from JNI / the UI thread),
//   - reports surface size changes and destruction (Resize/SetShouldClose).
//
// This file is compiled only on mobile platforms (ANDROID or __APPLE__ with
// TARGET_OS_IPHONE). On desktop, glfw_bridge.cpp provides the implementation.
// =============================================================================

#include "neoflux/renderers/platform_bridge.h"

#if defined(ANDROID) || (defined(__APPLE__) && defined(TARGET_OS_IPHONE))

#include <glog/logging.h>

#if defined(ANDROID)
#include <android/native_window.h>
#endif

namespace neoflux {
namespace {

// Mobile platform bridge. Holds a reference to the platform-provided native
// surface; the GPU context/surface itself lives inside tgfx (EGLWindow /
// EAGLWindow). See the file header for the ownership split.
class MobileBridge final : public PlatformBridge {
 public:
  // Constructs a mobile bridge from a native surface handle.
  //   Android: ANativeWindow* obtained from the NativeActivity or SurfaceView.
  //   iOS:     CAEAGLLayer* from the view hierarchy (via the ObjC++ shell).
  explicit MobileBridge(void* native_surface, int width, int height)
      : native_surface_(native_surface), width_(width), height_(height) {
    if (native_surface_ == nullptr) {
      LOG(ERROR) << "MobileBridge: null native surface from the app shell";
    }
#if defined(ANDROID)
    auto* window = static_cast<ANativeWindow*>(native_surface_);
    if (window != nullptr) {
      // Keep the surface dimensions in sync with what the shell reported.
      width_ = ANativeWindow_getWidth(window);
      height_ = ANativeWindow_getHeight(window);
    }
#endif
    LOG(INFO) << "MobileBridge created for " << width_ << "x" << height_
              << " surface (rendering context owned by tgfx)";
  }

  ~MobileBridge() override = default;

  [[nodiscard]] int GetWidth() const noexcept override { return width_; }
  [[nodiscard]] int GetHeight() const noexcept override { return height_; }

  // The native window handle (ANativeWindow* / CAEAGLLayer*) handed to
  // TgfxRenderer::Init(), which wraps it in the tgfx Window.
  [[nodiscard]] void* GetNativeHandle() const noexcept override {
    return native_surface_;
  }

  void SetInputCallback(InputEventCallback callback) override {
    input_callback_ = std::move(callback);
  }

  void PollEvents() override {
    // Touch events are pushed asynchronously via DispatchTouchEvent() from
    // the platform shell (JNI / UI thread); nothing to poll.
  }

  [[nodiscard]] bool ShouldClose() const noexcept override {
    return should_close_;
  }

  // Called by the platform shell (JNI / UIKit) when a touch event occurs.
  // Converts the platform touch into a NeoFlux input event and dispatches it
  // to the callback wired by Application::Init().
  void DispatchTouchEvent(MouseButton button, InputAction action,
                          float x, float y) {
    if (input_callback_) {
      input_callback_(button, action, Point{.x = x, .y = y});
    }
  }

  // Called by the platform when the surface is destroyed (e.g. app paused).
  void SetShouldClose(bool value) noexcept { should_close_ = value; }

  // Called by the platform when the window size changes (rotation, etc.).
  // The tgfx Window picks the new swapchain size up on the next frame.
  void Resize(int width, int height) noexcept {
    width_ = width;
    height_ = height;
  }

 private:
  void* native_surface_ = nullptr;
  int width_ = 0;
  int height_ = 0;
  bool should_close_ = false;
  InputEventCallback input_callback_;
};

}  // namespace

// Factory used by RenderLayer::Start() on mobile builds (the mobile
// equivalent of the desktop GlfwBridge).
std::unique_ptr<PlatformBridge> CreateMobileBridge(void* native_surface,
                                                    int width, int height) {
  return std::make_unique<MobileBridge>(native_surface, width, height);
}

}  // namespace neoflux

#endif  // ANDROID || TARGET_OS_IPHONE
