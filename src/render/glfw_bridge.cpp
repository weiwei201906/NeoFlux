// =============================================================================
// NeoFlux - glfw_bridge.cpp
//
// Implementation of GlfwBridge. Methods moved from header.
// =============================================================================

#include "neoflux/render/glfw_bridge.h"

#ifdef NEOFLUX_PLATFORM_DESKTOP

#include <string>
#include <string_view>
#include <utility>

#include <glog/logging.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace neoflux {

namespace {

struct WindowUserData {
  GlfwBridge* bridge = nullptr;
};

}  // namespace

GlfwBridge::GlfwBridge() : window_(nullptr), initialized_(false) {}

GlfwBridge::~GlfwBridge() { Shutdown(); }

bool GlfwBridge::Init(int width, int height, std::string_view title) {
  if (initialized_) {
    LOG(WARNING) << "GlfwBridge already initialized";
    return false;
  }

  glfwSetErrorCallback(ErrorCallback);

  if (glfwInit() == GLFW_FALSE) {
    LOG(ERROR) << "Failed to initialize GLFW";
    return false;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_FALSE);

  const std::string title_str(title);  // NOLINT(bugprone-unused-local-non-trivial-variable)
  window_ = glfwCreateWindow(width, height, title_str.c_str(), nullptr,
                             nullptr);
  if (window_ == nullptr) {
    LOG(ERROR) << "Failed to create GLFW window";
    glfwTerminate();
    return false;
  }

  auto* user_data = new WindowUserData{this};
  glfwSetWindowUserPointer(window_, user_data);

  // Context is made current later in the render thread via MakeContextCurrent().
  // This allows the render thread to own the GL context exclusively.

  glfwSetFramebufferSizeCallback(window_, FramebufferSizeCallback);
  glfwSetKeyCallback(window_, KeyCallback);
  glfwSetMouseButtonCallback(window_, MouseButtonCallback);
  glfwSetCursorPosCallback(window_, CursorPosCallback);
  glfwSetScrollCallback(window_, ScrollCallback);

  initialized_ = true;
  LOG(INFO) << "GLFW window created: " << width << "x" << height;
  return true;
}

void GlfwBridge::Shutdown() {
  if (!initialized_) {
    return;
  }

  if (window_ != nullptr) {
    auto* user_data =
        static_cast<WindowUserData*>(glfwGetWindowUserPointer(window_));
    delete user_data;

    glfwDestroyWindow(window_);
    window_ = nullptr;
  }

  glfwTerminate();
  initialized_ = false;
  LOG(INFO) << "GLFW bridge shut down";
}

void GlfwBridge::PollEvents() const {
  if (initialized_) {
    glfwPollEvents();
  }
}

void GlfwBridge::SwapBuffers() {
  if (window_ != nullptr) {
    glfwSwapBuffers(window_);
  }
}

void GlfwBridge::MakeContextCurrent() {
  if (window_ != nullptr) {
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);
  }
}

void GlfwBridge::ReleaseContext() {
  glfwMakeContextCurrent(nullptr);
}

bool GlfwBridge::ShouldClose() const {
  return window_ != nullptr && glfwWindowShouldClose(window_) != 0;
}

void GlfwBridge::GetFramebufferSize(int& width, int& height) const {
  if (window_ != nullptr) {
    glfwGetFramebufferSize(window_, &width, &height);
  } else {
    width = 0;
    height = 0;
  }
}

void GlfwBridge::GetWindowSize(int& width, int& height) const {
  if (window_ != nullptr) {
    glfwGetWindowSize(window_, &width, &height);
  } else {
    width = 0;
    height = 0;
  }
}

GLFWwindow* GlfwBridge::GetNativeHandle() const noexcept { return window_; }

Point GlfwBridge::GetCursorPos() const noexcept {
  if (window_ == nullptr) {
    return {.x = 0.0F, .y = 0.0F};
  }
  double xpos = 0.0;
  double ypos = 0.0;
  glfwGetCursorPos(window_, &xpos, &ypos);
  return {.x = static_cast<float>(xpos), .y = static_cast<float>(ypos)};
}

void* GlfwBridge::GetGlContext() const noexcept {
  return static_cast<void*>(window_);
}

void GlfwBridge::SetInputCallback(InputEventCallback callback) noexcept {
  input_callback_ = std::move(callback);
}

void GlfwBridge::SetScrollCallback(ScrollEventCallback callback) noexcept {
  scroll_callback_ = std::move(callback);
}

void GlfwBridge::SetResizeCallback(ResizeCallback callback) noexcept {
  resize_callback_ = std::move(callback);
}

void GlfwBridge::SetMouseMoveCallback(MouseMoveCallback callback) noexcept {
  mouse_move_callback_ = std::move(callback);
}

void GlfwBridge::ErrorCallback(int error, const char* description) {
  LOG(ERROR) << "GLFW error " << error << ": "
             << (description != nullptr ? description : "unknown");
}

void GlfwBridge::FramebufferSizeCallback(GLFWwindow* window, int width,
                                         int height) {
  auto* user_data =
      static_cast<WindowUserData*>(glfwGetWindowUserPointer(window));
  if (user_data == nullptr || user_data->bridge == nullptr) {
    return;
  }
  VLOG(1) << "Framebuffer resized: " << width << "x" << height;
  if (user_data->bridge->resize_callback_) {
    user_data->bridge->resize_callback_(width, height);
  }
}

void GlfwBridge::KeyCallback(GLFWwindow* /*window*/, int key,
                             int /*scancode*/, int action, int /*mods*/) {
  VLOG(2) << "Key event: key=" << key << " action=" << action;
}

void GlfwBridge::MouseButtonCallback(GLFWwindow* window, int button,
                                     int action, int /*mods*/) {
  auto* user_data =
      static_cast<WindowUserData*>(glfwGetWindowUserPointer(window));
  if (user_data == nullptr || user_data->bridge == nullptr) {
    return;
  }
  auto* bridge = user_data->bridge;
  if (!bridge->input_callback_) {
    return;
  }
  // Query cursor position directly instead of relying on the cached value from
  // CursorPosCallback, which may be stale if the button is pressed without
  // prior mouse movement.
  double cursor_x = 0.0;
  double cursor_y = 0.0;
  glfwGetCursorPos(window, &cursor_x, &cursor_y);
  bridge->last_cursor_x_ = cursor_x;
  bridge->last_cursor_y_ = cursor_y;
  const auto btn = static_cast<MouseButton>(button);
  const auto act = static_cast<InputAction>(action);
  bridge->input_callback_(btn, act,
                          {.x = static_cast<float>(cursor_x),
                           .y = static_cast<float>(cursor_y),});
}

void GlfwBridge::CursorPosCallback(GLFWwindow* window, double xpos,
                                   double ypos) {
  auto* user_data =
      static_cast<WindowUserData*>(glfwGetWindowUserPointer(window));
  if (user_data == nullptr || user_data->bridge == nullptr) {
    return;
  }
  user_data->bridge->last_cursor_x_ = xpos;
  user_data->bridge->last_cursor_y_ = ypos;
  if (user_data->bridge->mouse_move_callback_ != nullptr) {
    user_data->bridge->mouse_move_callback_(
        {.x = static_cast<float>(xpos), .y = static_cast<float>(ypos)});
  }
}

void GlfwBridge::ScrollCallback(GLFWwindow* window, double xoffset,
                                double yoffset) {
  auto* user_data =
      static_cast<WindowUserData*>(glfwGetWindowUserPointer(window));
  if (user_data == nullptr || user_data->bridge == nullptr) {
    return;
  }
  if (user_data->bridge->scroll_callback_ != nullptr) {
    user_data->bridge->scroll_callback_(xoffset, yoffset);
  }
}

}  // namespace neoflux

#else  // !NEOFLUX_PLATFORM_DESKTOP

namespace neoflux {

GlfwBridge::GlfwBridge() : window_(nullptr), initialized_(false) {}

GlfwBridge::~GlfwBridge() = default;

bool GlfwBridge::Init(int /*width*/, int /*height*/,
                      std::string_view /*title*/) {
  return false;
}

void GlfwBridge::Shutdown() {}

void GlfwBridge::PollEvents() {}

void GlfwBridge::SwapBuffers() {}

bool GlfwBridge::ShouldClose() const { return false; }

void GlfwBridge::GetFramebufferSize(int& width, int& height) const {
  width = 0;
  height = 0;
}

void GlfwBridge::GetWindowSize(int& width, int& height) const {
  width = 0;
  height = 0;
}

GLFWwindow* GlfwBridge::GetNativeHandle() const noexcept { return nullptr; }

Point GlfwBridge::GetCursorPos() const noexcept {
  return {.x = 0.0F, .y = 0.0F};
}

void* GlfwBridge::GetGlContext() const noexcept { return nullptr; }

void GlfwBridge::ErrorCallback(int /*error*/, const char* /*description*/) {}

void GlfwBridge::FramebufferSizeCallback(GLFWwindow* /*window*/, int /*width*/,
                                         int /*height*/) {}

void GlfwBridge::KeyCallback(GLFWwindow* /*window*/, int /*key*/,
                             int /*scancode*/, int /*action*/, int /*mods*/) {}

void GlfwBridge::MouseButtonCallback(GLFWwindow* /*window*/, int /*button*/,
                                     int /*action*/, int /*mods*/) {}

void GlfwBridge::CursorPosCallback(GLFWwindow* /*window*/, double /*xpos*/,
                                   double /*ypos*/) {}

void GlfwBridge::ScrollCallback(GLFWwindow* /*window*/, double /*xoffset*/,
                                double /*yoffset*/) {}

}  // namespace neoflux

#endif  // NEOFLUX_PLATFORM_DESKTOP
