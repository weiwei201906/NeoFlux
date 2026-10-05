// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - Platform Bridge
//
// Abstract interface for platform-specific window/input management.
// Desktop implementations use GLFW; mobile implementations use the native
// platform API (ANativeWindow on Android, UIView/CALayer on iOS).
//
// Bridges are pure window + input carriers: the graphics surface, context,
// and presentation belong to the tgfx Window (see TgfxRenderer). The render
// layer owns a PlatformBridge and uses it to:
//   - Query window dimensions and the native window handle
//   - Poll platform events
//   - Dispatch input events to the application
// =============================================================================

#ifndef NEOFLUX_RENDER_PLATFORM_BRIDGE_H_
#define NEOFLUX_RENDER_PLATFORM_BRIDGE_H_

#include <cstdint>
#include <functional>
#include <memory>

#include "neoflux/core/types.h"

namespace neoflux {

// Input action types (mouse button / touch).
enum class InputAction : std::uint8_t {
  kPress = 0,
  kRelease = 1,
  kMove = 2,
  kRepeat = 3,  // Desktop only: GLFW keyboard/mouse auto-repeat.
};

// Mouse / touch button identifiers.
enum class MouseButton : std::uint8_t {
  kLeft = 0,
  kRight = 1,
  kMiddle = 2,
  kTouch = 3,  // Single-finger touch on mobile.
};

// Callback signature for input events. The point is in pixel coordinates
// relative to the window's top-left corner.
using InputEventCallback =
    std::function<void(MouseButton button, InputAction action, const Point& pos)>;

// Abstract platform bridge. Concrete implementations exist for desktop
// (GLFW) and mobile (Android/iOS native surfaces).
class PlatformBridge {
 public:
  virtual ~PlatformBridge() = default;

  // Returns the current window width in pixels.
  [[nodiscard]] virtual int GetWidth() const noexcept = 0;

  // Returns the current window height in pixels.
  [[nodiscard]] virtual int GetHeight() const noexcept = 0;

  // Returns the native window handle (GLFWwindow* on desktop,
  // ANativeWindow* on Android, UIView*/CALayer* on iOS).
  [[nodiscard]] virtual void* GetNativeHandle() const noexcept = 0;

  // Sets the callback invoked when input events arrive.
  virtual void SetInputCallback(InputEventCallback callback) = 0;

  // Polls for pending platform events (non-blocking). Called from the
  // main thread event loop.
  virtual void PollEvents() = 0;

  // Returns true if the window has been closed by the user.
  [[nodiscard]] virtual bool ShouldClose() const noexcept = 0;

  // --- Shell-driven hooks (mobile) ---------------------------------------
  // On mobile there is no windowing system to poll: the surface is owned by
  // the app shell, which pushes state into the bridge from its own thread.
  // These three entry points must therefore live on this interface, because
  // CreateMobileBridge() hands callers a PlatformBridge* (via
  // RenderLayer::GetPlatformBridge()) and nothing else could reach them.
  //
  // They are pure virtual rather than defaulted no-ops: silently dropping
  // shell input is exactly how the mobile path became unusable, and there is
  // one implementation in this project (MobileBridge) plus no desktop one --
  // GlfwBridge is NOT a PlatformBridge -- so nothing else needs updating.
  // DispatchTouchEvent() in particular is deliberately not noexcept, since it
  // runs the callback registered by SetInputCallback(), i.e. application code.
  //
  // Threading contract: the shell drives these and this class has no internal
  // locking, so they must be called from the same thread that runs the
  // application event loop. That is the thread the platform delivers touch on
  // (the JNI / UIKit thread), which is also the thread Application::Init()
  // and Application::Run() are expected to run on.

  // Delivers a touch (or other pointer) event from the platform shell to the
  // widget tree. |pos| is in surface pixel coordinates relative to the
  // surface's top-left corner; the desktop path uses the same convention for
  // cursor positions, so Application::DispatchPointerEvent() scales them
  // identically. No-op when no input callback is registered.
  virtual void DispatchTouchEvent(MouseButton button, InputAction action,
                                  const Point& pos) = 0;

  // Called by the shell when the surface is resized or destroyed, typically
  // on rotation or when the app is backgrounded. The renderer picks the new
  // swapchain size up on the next frame; RenderLayer::GetWindowSize() reports
  // these new values from then on.
  virtual void Resize(int width, int height) noexcept = 0;

  // Called by the shell when the surface goes away (app paused, surface
  // destroyed). Reported by ShouldClose(), which makes Application::OnFrame()
  // stop the run loop.
  virtual void SetShouldClose(bool value) noexcept = 0;
};

// Creates the mobile platform bridge (Android/iOS). |native_surface| is the
// ANativeWindow* (Android) or CAEAGLLayer*/UIView* (iOS) handed over by the
// platform shell. The rendering context itself is NOT owned by the bridge:
// tgfx::EGLWindow / EAGLWindow create and own the EGL/EAGL context and
// surface for |native_surface| (see TgfxRenderer). The bridge only carries
// the native handle and dispatches touch input into the widget tree.
// Returns nullptr on unsupported platforms.
std::unique_ptr<PlatformBridge> CreateMobileBridge(void* native_surface,
                                                   int width, int height);

}  // namespace neoflux

#endif  // NEOFLUX_RENDER_PLATFORM_BRIDGE_H_
