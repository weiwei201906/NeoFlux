// =============================================================================
// NeoFlux - glfw_bridge.h
//
// GLFW bridge for desktop platforms. All method implementations are in
// glfw_bridge.cpp.
// =============================================================================

#ifndef NEOFLUX_RENDER_GLFW_BRIDGE_H_
#define NEOFLUX_RENDER_GLFW_BRIDGE_H_

#include <functional>
#include <string_view>

#include "neoflux/core/noncopyable.h"
#include "neoflux/core/types.h"

// Forward declaration of GLFW window to avoid including GLFW headers here.
struct GLFWwindow;

namespace neoflux {

// Mouse button identifiers (matches GLFW constants).
enum class MouseButton : std::uint8_t { kLeft = 0, kRight = 1, kMiddle = 2 };

// Input action identifiers (matches GLFW constants).
enum class InputAction : std::uint8_t { kPress = 1, kRelease = 0, kRepeat = 2 };

// Callback type for input events. Receives button, action, and cursor position
// in window coordinates (pixels, origin at top-left).
using InputEventCallback =
    std::function<void(MouseButton button, InputAction action, const Point& pos)>;

// Desktop window and input bridge using GLFW.
class GlfwBridge : public NonCopyable {
 public:
  GlfwBridge();
  ~GlfwBridge();

  // Initializes GLFW and creates a window.
  bool Init(int width, int height, std::string_view title);

  // Destroys the window and shuts down GLFW.
  void Shutdown();

  // Polls for window and input events (non-blocking).
  void PollEvents() const;

  // Swaps the front and back buffers (presents the frame).
  void SwapBuffers();

  // Returns true if the window has been requested to close.
  [[nodiscard]] bool ShouldClose() const;

  // Returns the window's framebuffer size in pixels.
  void GetFramebufferSize(int& width, int& height) const;

  // Returns the native window handle (GLFWwindow*).
  [[nodiscard]] GLFWwindow* GetNativeHandle() const noexcept;

  // Returns the OpenGL context (for tgfx initialization).
  [[nodiscard]] void* GetGlContext() const noexcept;

  // Makes the OpenGL context current on the calling thread.
  void MakeContextCurrent();

  // Releases the OpenGL context from the calling thread.
  void ReleaseContext();

  // Sets the callback invoked for mouse button events.
  void SetInputCallback(InputEventCallback callback) noexcept;

 private:
  static void ErrorCallback(int error, const char* description);
  static void FramebufferSizeCallback(GLFWwindow* window, int width,
                                      int height);
  static void KeyCallback(GLFWwindow* window, int key, int scancode,
                          int action, int mods);
  static void MouseButtonCallback(GLFWwindow* window, int button, int action,
                                  int mods);
  static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);

  GLFWwindow* window_;
  bool initialized_;
  InputEventCallback input_callback_;
  double last_cursor_x_ = 0.0;
  double last_cursor_y_ = 0.0;
};

}  // namespace neoflux

#endif  // NEOFLUX_RENDER_GLFW_BRIDGE_H_
