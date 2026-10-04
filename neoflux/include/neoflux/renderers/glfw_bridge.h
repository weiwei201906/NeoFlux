// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - glfw_bridge.h
//
// GLFW window/input bridge for desktop. Pimpl: all state (GLFWwindow*,
// callbacks, cursor cache) lives in struct Impl defined in glfw_bridge.cpp.
//
// GLFW is a pure window + input bridge: the window is created with
// GLFW_NO_API and all GL/EGL/WGL context management belongs to the tgfx
// Window (see TgfxRenderer). There is deliberately no context or swap API
// here.
// =============================================================================

#ifndef NEOFLUX_RENDER_GLFW_BRIDGE_H_
#define NEOFLUX_RENDER_GLFW_BRIDGE_H_

#include <functional>
#include <memory>
#include <string_view>

#include "neoflux/core/noncopyable.h"
#include "neoflux/core/types.h"
#include "neoflux/renderers/platform_bridge.h"

// Forward declaration of GLFW window to avoid including GLFW headers here.
struct GLFWwindow;

namespace neoflux {

// MouseButton / InputAction / InputEventCallback come from platform_bridge.h
// (single definition shared with the mobile bridge). NOTE: the GLFW numeric
// constants are NOT the enum values -- glfw_bridge.cpp maps them explicitly.

// Callback type for mouse scroll events.
using ScrollEventCallback = std::function<void(double xoffset, double yoffset)>;

// Callback type for framebuffer resize events.
using ResizeCallback = std::function<void(int width, int height)>;

// Callback type for mouse cursor move events.
using MouseMoveCallback = std::function<void(const Point& pos)>;

// Desktop window and input bridge using GLFW. Copy/move are deleted by the
// NonCopyable base, so the special-member-functions rule is satisfied there.
class GlfwBridge : public NonCopyable {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  GlfwBridge();
  ~GlfwBridge();

  // Initializes GLFW and creates a window.
  bool Init(int width, int height, std::string_view title);

  // Destroys the window and shuts down GLFW.
  void Shutdown() noexcept;

  // Polls for window and input events (non-blocking).
  void PollEvents() const;

  // Returns true if the window has been requested to close.
  [[nodiscard]] bool ShouldClose() const;

  // Returns the window's framebuffer size in pixels.
  void GetFramebufferSize(int& width, int& height) const;

  // Returns the window client-area size in screen coordinates.
  void GetWindowSize(int& width, int& height) const;

  // Returns the native window handle (GLFWwindow*).
  [[nodiscard]] GLFWwindow* GetNativeHandle() const noexcept;

  // Returns the current cursor position in window coordinates.
  [[nodiscard]] Point GetCursorPos() const noexcept;

  // Sets the callback invoked for mouse button events.
  void SetInputCallback(InputEventCallback callback) noexcept;

  // Sets the callback invoked for mouse scroll events.
  void SetScrollCallback(ScrollEventCallback callback) noexcept;

  // Sets the callback invoked when the framebuffer is resized.
  void SetResizeCallback(ResizeCallback callback) noexcept;

  // Sets the callback invoked when the mouse cursor moves.
  void SetMouseMoveCallback(MouseMoveCallback callback) noexcept;

 private:
  static void ErrorCallback(int error, const char* description);
  static void FramebufferSizeCallback(GLFWwindow* window, int width,
                                      int height);
  static void KeyCallback(GLFWwindow* window, int key, int scancode,
                          int action, int mods);
  static void MouseButtonCallback(GLFWwindow* window, int button, int action,
                                  int mods);
  static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
  static void ScrollCallback(GLFWwindow* window, double xoffset,
                             double yoffset);

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace neoflux

#endif  // NEOFLUX_RENDER_GLFW_BRIDGE_H_
