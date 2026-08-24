// =============================================================================
// NeoFlux - Mobile Platform Bridge
//
// Platform bridge implementation for mobile (Android / iOS). Unlike the
// desktop GLFW bridge, mobile does not create its own window; the platform
// provides a native surface (ANativeWindow on Android, CAMetalLayer /
// CAEAGLLayer on iOS) that the renderer draws into.
//
// Input events are forwarded from the platform's touch system via
// DispatchTouchEvent().
//
// This file is compiled only on mobile platforms (ANDROID or __APPLE__ with
// TARGET_OS_IPHONE). On desktop, glfw_bridge.cpp provides the implementation.
// =============================================================================

#include "neoflux/render/platform_bridge.h"

#if defined(ANDROID) || (defined(__APPLE__) && defined(TARGET_OS_IPHONE))

#include <glog/logging.h>

#if defined(ANDROID)
#include <android/native_window.h>
#include <EGL/egl.h>
#endif

namespace neoflux {
namespace {

// Mobile platform bridge. Owns the EGL context (Android) or references the
// UIKit-provided context (iOS). The native surface handle is provided by the
// platform at construction time.
class MobileBridge final : public PlatformBridge {
 public:
  // Constructs a mobile bridge from a native surface handle.
  //   Android: ANativeWindow* obtained from the NativeActivity or SurfaceView.
  //   iOS:     UIView* or CAMetalLayer* from the view hierarchy.
  explicit MobileBridge(void* native_surface, int width, int height)
      : native_surface_(native_surface), width_(width), height_(height) {
#if defined(ANDROID)
    InitializeEGL();
#endif
  }

  ~MobileBridge() override {
#if defined(ANDROID)
    if (egl_surface_ != EGL_NO_SURFACE) {
      eglDestroySurface(egl_display_, egl_surface_);
    }
    if (egl_context_ != EGL_NO_CONTEXT) {
      eglDestroyContext(egl_display_, egl_context_);
    }
    if (egl_display_ != EGL_NO_DISPLAY) {
      eglTerminate(egl_display_);
    }
#endif
  }

  void MakeContextCurrent() override {
#if defined(ANDROID)
    if (egl_display_ != EGL_NO_DISPLAY && egl_surface_ != EGL_NO_SURFACE) {
      eglMakeCurrent(egl_display_, egl_surface_, egl_surface_, egl_context_);
    }
#endif
    // iOS: the EAGL/Metal context is made current by the platform view.
  }

  void SwapBuffers() override {
#if defined(ANDROID)
    if (egl_display_ != EGL_NO_DISPLAY && egl_surface_ != EGL_NO_SURFACE) {
      eglSwapBuffers(egl_display_, egl_surface_);
    }
#endif
    // iOS: present is handled by tgfx or the Metal drawable.
  }

  [[nodiscard]] int GetWidth() const noexcept override { return width_; }
  [[nodiscard]] int GetHeight() const noexcept override { return height_; }
  [[nodiscard]] void* GetNativeHandle() const noexcept override {
    return native_surface_;
  }

  void SetInputCallback(InputEventCallback callback) override {
    input_callback_ = std::move(callback);
  }

  void SetKeyCallback(KeyEventCallback callback) override {
    key_callback_ = std::move(callback);
  }

  void SetCharCallback(CharEventCallback callback) override {
    char_callback_ = std::move(callback);
  }

  void PollEvents() override {
    // Mobile events are delivered asynchronously via DispatchTouchEvent;
    // no polling is needed.
  }

  [[nodiscard]] bool ShouldClose() const noexcept override {
    return should_close_;
  }

  // Called by the platform (JNI / UIKit) when a touch event occurs.
  // Converts the platform touch into a NeoFlux input event and dispatches it.
  void DispatchTouchEvent(MouseButton button, InputAction action,
                          float x, float y) {
    if (input_callback_) {
      input_callback_(button, action, Point{.x = x, .y = y});
    }
  }

  // Called by the platform when the surface is destroyed (e.g. app paused).
  void SetShouldClose(bool value) noexcept { should_close_ = value; }

  // Called by the platform when the window size changes (rotation, etc.).
  void Resize(int width, int height) noexcept {
    width_ = width;
    height_ = height;
  }

 private:
#if defined(ANDROID)
  // Creates an EGL context and surface for the ANativeWindow.
  void InitializeEGL() {
    auto* window = static_cast<ANativeWindow*>(native_surface_);
    if (window == nullptr) {
      LOG(ERROR) << "MobileBridge: null native window";
      return;
    }

    egl_display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl_display_ == EGL_NO_DISPLAY) {
      LOG(ERROR) << "MobileBridge: eglGetDisplay failed";
      return;
    }

    EGLint major = 0;
    EGLint minor = 0;
    if (!eglInitialize(egl_display_, &major, &minor)) {
      LOG(ERROR) << "MobileBridge: eglInitialize failed";
      return;
    }

    const EGLint config_attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_NONE,
    };
    EGLConfig config = nullptr;
    EGLint num_configs = 0;
    eglChooseConfig(egl_display_, config_attribs, &config, 1, &num_configs);
    if (config == nullptr) {
      LOG(ERROR) << "MobileBridge: eglChooseConfig failed";
      return;
    }

    egl_surface_ = eglCreateWindowSurface(egl_display_, config, window, nullptr);
    if (egl_surface_ == EGL_NO_SURFACE) {
      LOG(ERROR) << "MobileBridge: eglCreateWindowSurface failed";
      return;
    }

    const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE,
    };
    egl_context_ = eglCreateContext(egl_display_, config, EGL_NO_CONTEXT,
                                    context_attribs);
    if (egl_context_ == EGL_NO_CONTEXT) {
      LOG(ERROR) << "MobileBridge: eglCreateContext failed (GLES 3.0)";
      // Fall back to GLES 2.0.
      const EGLint ctx2_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
      egl_context_ = eglCreateContext(egl_display_, config, EGL_NO_CONTEXT,
                                      ctx2_attribs);
    }

    if (egl_context_ != EGL_NO_CONTEXT) {
      LOG(INFO) << "MobileBridge: EGL context created ("
                << major << "." << minor << ")";
    }
  }

  EGLDisplay egl_display_ = EGL_NO_DISPLAY;
  EGLSurface egl_surface_ = EGL_NO_SURFACE;
  EGLContext egl_context_ = EGL_NO_CONTEXT;
#endif  // ANDROID

  void* native_surface_ = nullptr;
  int width_ = 0;
  int height_ = 0;
  bool should_close_ = false;
  InputEventCallback input_callback_;
  KeyEventCallback key_callback_;
  CharEventCallback char_callback_;
};

}  // namespace

// Factory function used by the application layer to create a mobile bridge.
// This is the mobile equivalent of GlfwBridge::Create().
std::unique_ptr<PlatformBridge> CreateMobileBridge(void* native_surface,
                                                    int width, int height) {
  return std::make_unique<MobileBridge>(native_surface, width, height);
}

}  // namespace neoflux

#else  // !ANDROID && !TARGET_OS_IPHONE

#include <glog/logging.h>

namespace neoflux {

// Desktop stub: mobile bridge is not available on desktop.
std::unique_ptr<PlatformBridge> CreateMobileBridge(void* /*native_surface*/,
                                                    int /*width*/,
                                                    int /*height*/) {
  LOG(WARNING) << "CreateMobileBridge called on desktop platform; returning null";
  return nullptr;
}

}  // namespace neoflux

#endif  // ANDROID || TARGET_OS_IPHONE
