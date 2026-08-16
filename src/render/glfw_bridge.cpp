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
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

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

  glfwMakeContextCurrent(window_);
  glfwSwapInterval(1);

  glfwSetFramebufferSizeCallback(window_, FramebufferSizeCallback);
  glfwSetKeyCallback(window_, KeyCallback);
  glfwSetMouseButtonCallback(window_, MouseButtonCallback);
  glfwSetCursorPosCallback(window_, CursorPosCallback);

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

GLFWwindow* GlfwBridge::GetNativeHandle() const noexcept { return window_; }

void* GlfwBridge::GetGlContext() const noexcept {
  return static_cast<void*>(window_);
}

void GlfwBridge::ErrorCallback(int error, const char* description) {
  LOG(ERROR) << "GLFW error " << error << ": "
             << (description != nullptr ? description : "unknown");
}

void GlfwBridge::FramebufferSizeCallback(GLFWwindow* /*window*/, int width,
                                         int height) {
  VLOG(1) << "Framebuffer resized: " << width << "x" << height;
}

void GlfwBridge::KeyCallback(GLFWwindow* /*window*/, int key,
                             int /*scancode*/, int action, int /*mods*/) {
  VLOG(2) << "Key event: key=" << key << " action=" << action;
}

void GlfwBridge::MouseButtonCallback(GLFWwindow* /*window*/, int button,
                                     int action, int /*mods*/) {
  VLOG(2) << "Mouse button: button=" << button << " action=" << action;
}

void GlfwBridge::CursorPosCallback(GLFWwindow* /*window*/, double xpos,
                                   double ypos) {
  VLOG(3) << "Cursor pos: " << xpos << ", " << ypos;
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

GLFWwindow* GlfwBridge::GetNativeHandle() const noexcept { return nullptr; }

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

}  // namespace neoflux

#endif  // NEOFLUX_PLATFORM_DESKTOP
