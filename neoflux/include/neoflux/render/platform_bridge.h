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
#include <memory>
#include <string>
#include <string_view>

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

// Keyboard modifier flags (bitmask).
enum class KeyModifiers : std::uint8_t {
  kNone = 0,
  kShift = 1 << 0,
  kControl = 1 << 1,
  kAlt = 1 << 2,
  kSuper = 1 << 3,
};

// Key codes (subset of GLFW key codes, platform-independent).
enum class KeyCode : std::int32_t {
  kUnknown = -1,
  kBackspace = 259,
  kTab = 258,
  kEnter = 257,
  kEscape = 256,
  kDelete = 261,
  kRight = 262,
  kLeft = 263,
  kDown = 264,
  kUp = 265,
  kHome = 268,
  kEnd = 269,
  kA = 65,
  kC = 67,
  kV = 86,
  kX = 88,
  kZ = 90,
};

// Keyboard key event.
struct KeyEvent {
  KeyCode key;
  std::uint8_t modifiers;  // Bitmask of KeyModifiers.
  bool pressed;            // true = press, false = release.
};

// Unicode character input event (after OS keyboard layout mapping).
struct CharEvent {
  std::uint32_t codepoint;  // Unicode code point (UTF-32).
};

// Callback signatures for keyboard events.
using KeyEventCallback = std::function<void(const KeyEvent& event)>;
using CharEventCallback = std::function<void(const CharEvent& event)>;

// Abstract native text input field. On desktop platforms this wraps the OS
// native text control (Win32 EDIT, GTK Entry, NSTextField) to get full IME,
// clipboard, caret, and selection support for free. On mobile platforms the
// widget renders its own text via tgfx (see TextField).
class NativeTextField {
 public:
  virtual ~NativeTextField() = default;

  // Sets the text content (UTF-8).
  virtual void SetText(std::string_view text) = 0;

  // Returns the current text content (UTF-8).
  [[nodiscard]] virtual std::string GetText() const = 0;

  // Sets the placeholder text shown when the field is empty (UTF-8).
  virtual void SetPlaceholder(std::string_view text) = 0;

  // Moves the control to the given position in window coordinates (pixels).
  virtual void SetPosition(float x, float y) = 0;

  // Resizes the control to the given dimensions (pixels).
  virtual void SetSize(float width, float height) = 0;

  // Sets the font size in pixels.
  virtual void SetFontSize(float size) = 0;

  // Gives keyboard focus to the native control.
  virtual void SetFocus() = 0;

  // Shows the control.
  virtual void Show() = 0;

  // Hides the control.
  virtual void Hide() = 0;

  // Registers a callback invoked whenever the text changes. The callback
  // receives the new text (UTF-8).
  virtual void SetOnTextChanged(
      std::function<void(std::string_view)> callback) = 0;
};

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

  // Sets the callback invoked when keyboard key events arrive.
  virtual void SetKeyCallback(KeyEventCallback callback) = 0;

  // Sets the callback invoked when Unicode character input arrives.
  virtual void SetCharCallback(CharEventCallback callback) = 0;

  // Polls for pending platform events (non-blocking). Called from the
  // main thread event loop.
  virtual void PollEvents() = 0;

  // Returns true if the window has been closed by the user.
  [[nodiscard]] virtual bool ShouldClose() const noexcept = 0;

  // Mouse cursor types.
  enum class CursorType : std::uint8_t {
    kArrow = 0,   // Standard arrow pointer.
    kIBeam = 1,   // Text input cursor.
    kHand = 2,    // Clickable link/button cursor.
    kResize = 3,  // Resize/move cursor.
  };

  // Sets the mouse cursor shape. Called when pointer enters/leaves widgets
  // that request a specific cursor (e.g. TextField -> I-beam).
  virtual void SetCursor(CursorType type) = 0;

  // Returns the current clipboard text (UTF-8). Empty string if clipboard
  // is empty or contains non-text data.
  [[nodiscard]] virtual std::string GetClipboardText() const = 0;

  // Sets the clipboard text (UTF-8).
  virtual void SetClipboardText(std::string_view text) = 0;

  // Creates a platform-native text input field. Returns nullptr on platforms
  // that do not support native text fields (mobile uses tgfx-rendered fields).
  [[nodiscard]] virtual std::unique_ptr<NativeTextField>
  CreateNativeTextField() = 0;

  // Called when the platform destroys the rendering surface (e.g. Android
  // app sent to background, screen off). The EGL/Metal surface is invalidated;
  // subsequent MakeContextCurrent() will fail until OnSurfaceCreated() is
  // called. The GL context and resources (textures, shaders) are preserved.
  // Default implementation does nothing (desktop windows are not destroyed
  // by the OS in this way).
  virtual void OnSurfaceDestroyed() {}

  // Called when the platform recreates the rendering surface (e.g. Android
  // app brought to foreground). `new_surface` is the new native surface handle
  // (ANativeWindow* on Android, CAMetalLayer* on iOS). Re-creates the EGL
  // surface from the new native window. Returns true on success.
  // Default implementation does nothing and returns true (desktop).
  virtual bool OnSurfaceCreated(void* /*new_surface*/) { return true; }
};

}  // namespace neoflux

#endif  // NEOFLUX_RENDER_PLATFORM_BRIDGE_H_
