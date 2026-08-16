// =============================================================================
// NeoFlux - Platform Bridge
//
// Abstract interface for platform-specific window/context management.
// Desktop implementations use GLFW; mobile implementations use the native
// platform API (ANativeWindow on Android, CAMetalLayer/EAGL on iOS).
//
// The render layer owns a PlatformBridge and uses it to:
//   - Make the GL context current on the render thread
//   - Swap the front/back buffers
//   - Query window dimensions
//   - Dispatch input events to the application
// =============================================================================

#ifndef NEOFLUX_RENDER_PLATFORM_BRIDGE_H_
#define NEOFLUX_RENDER_PLATFORM_BRIDGE_H_

#include <cstdint>
#include <functional>

#include "neoflux/core/types.h"

namespace neoflux {

// Input action types (mouse button / touch).
enum class InputAction : std::uint8_t {
  kPress = 0,
  kRelease = 1,
  kMove = 2,
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

  // Makes the platform's rendering context current on the calling thread.
  // Must be called on the render thread before any GL/tgfx operations.
  virtual void MakeContextCurrent() = 0;

  // Swaps the front and back buffers (presents the rendered frame).
  virtual void SwapBuffers() = 0;

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
};

}  // namespace neoflux

#endif  // NEOFLUX_RENDER_PLATFORM_BRIDGE_H_
