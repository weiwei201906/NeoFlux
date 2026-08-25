// =============================================================================
// NeoFlux - glfw_bridge.h
//
// GLFW bridge for desktop platforms. Implements the PlatformBridge interface
// for window/context management on Windows, Linux, and macOS.
// All method implementations are in glfw_bridge.cpp.
// =============================================================================

#ifndef NEOFLUX_RENDER_GLFW_BRIDGE_H_
#define NEOFLUX_RENDER_GLFW_BRIDGE_H_

#include <functional>
#include <string_view>

#include "neoflux/core/types.h"
#include "neoflux/render/platform_bridge.h"

// Forward declaration of GLFW window to avoid including GLFW headers here.
struct GLFWwindow;
struct GLFWcursor;

namespace neoflux {

// Callback type for mouse scroll events. Receives the scroll delta in
// normalized units (positive = up/right).
using ScrollEventCallback = std::function<void(double xoffset, double yoffset)>;

// Callback type for framebuffer resize events. Receives the new framebuffer
// size in pixels.
using ResizeCallback = std::function<void(int width, int height)>;

// Callback type for mouse cursor move events. Receives the new cursor position
// in window coordinates (pixels, origin at top-left).
using MouseMoveCallback = std::function<void(const Point& pos)>;

// Desktop window and input bridge using GLFW.
// Implements PlatformBridge so the render layer can treat desktop and mobile
// uniformly.
class GlfwBridge final : public PlatformBridge {
 public:
  GlfwBridge();
  ~GlfwBridge() override;

  // Initializes GLFW and creates a window.
  bool Init(int width, int height, std::string_view title);

  // Destroys the window and shuts down GLFW.
  void Shutdown();

  // --- PlatformBridge overrides ---
  void MakeContextCurrent() override;
  void SwapBuffers() override;
  [[nodiscard]] int GetWidth() const noexcept override;
  [[nodiscard]] int GetHeight() const noexcept override;
  [[nodiscard]] void* GetNativeHandle() const noexcept override;
  void SetInputCallback(InputEventCallback callback) override;
  void SetKeyCallback(KeyEventCallback callback) override;
  void SetCharCallback(CharEventCallback callback) override;
  void PollEvents() override;
  [[nodiscard]] bool ShouldClose() const noexcept override;
  void SetCursor(CursorType type) override;
  [[nodiscard]] std::string GetClipboardText() const override;
  void SetClipboardText(std::string_view text) override;
  [[nodiscard]] std::unique_ptr<NativeTextField> CreateNativeTextField()
      override;

  // --- GLFW-specific methods ---

  // Returns the window's framebuffer size in pixels.
  void GetFramebufferSize(int& width, int& height) const;

  // Returns the window client-area size in screen coordinates. This may
  // differ from the requested size due to DPI virtualisation on Windows.
  void GetWindowSize(int& width, int& height) const;

  // Returns the GLFW window handle (typed, desktop-only).
  [[nodiscard]] GLFWwindow* GetGlfwWindow() const noexcept;

  // Returns the current cursor position in window coordinates.
  [[nodiscard]] Point GetCursorPos() const noexcept;

  // Returns the OpenGL context (for tgfx initialization).
  [[nodiscard]] void* GetGlContext() const noexcept;

  // Releases the OpenGL context from the calling thread.
  static void ReleaseContext();

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
  static void CharCallback(GLFWwindow* window, unsigned int codepoint);
  static void MouseButtonCallback(GLFWwindow* window, int button, int action,
                                  int mods);
  static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
  static void ScrollCallback(GLFWwindow* window, double xoffset,
                             double yoffset);

  GLFWwindow* window_ = nullptr;
  bool initialized_ = false;
  InputEventCallback input_callback_{};
  KeyEventCallback key_callback_{};
  CharEventCallback char_callback_{};
  ScrollEventCallback scroll_callback_{};
  ResizeCallback resize_callback_{};
  MouseMoveCallback mouse_move_callback_{};
  double last_cursor_x_ = 0.0;
  double last_cursor_y_ = 0.0;
  // Cached standard cursors (lazily created, freed in destructor).
  GLFWcursor* arrow_cursor_ = nullptr;
  GLFWcursor* ibeam_cursor_ = nullptr;
  GLFWcursor* hand_cursor_ = nullptr;
  CursorType current_cursor_ = CursorType::kArrow;
};

}  // namespace neoflux

#endif  // NEOFLUX_RENDER_GLFW_BRIDGE_H_
