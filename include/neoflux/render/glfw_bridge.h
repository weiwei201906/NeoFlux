// =============================================================================
// NeoFlux - glfw_bridge.h
//
// GLFW bridge for desktop platforms. All method implementations are in
// glfw_bridge.cpp.
// =============================================================================

#ifndef NEOFLUX_RENDER_GLFW_BRIDGE_H_
#define NEOFLUX_RENDER_GLFW_BRIDGE_H_

#include <string_view>

#include "neoflux/core/noncopyable.h"

// Forward declaration of GLFW window to avoid including GLFW headers here.
struct GLFWwindow;

namespace neoflux {

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
  void PollEvents();

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
};

}  // namespace neoflux

#endif  // NEOFLUX_RENDER_GLFW_BRIDGE_H_
