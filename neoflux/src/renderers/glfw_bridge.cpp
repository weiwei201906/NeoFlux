// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - glfw_bridge.cpp
//
// Implementation of GlfwBridge (Pimpl). All state lives in GlfwBridge::Impl.
// =============================================================================

#include "neoflux/renderers/glfw_bridge.h"

#ifdef NEOFLUX_PLATFORM_DESKTOP

#include <string>
#include <string_view>
#include <utility>

#include <glog/logging.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace neoflux {

struct GlfwBridge::Impl {
  GLFWwindow* window = nullptr;
  bool initialized = false;

  InputEventCallback input_callback;
  ScrollEventCallback scroll_callback;
  ResizeCallback resize_callback;
  MouseMoveCallback mouse_move_callback;

  double last_cursor_x = 0.0;
  double last_cursor_y = 0.0;
};

namespace {

struct WindowUserData {
  GlfwBridge* bridge = nullptr;
};

}  // namespace

GlfwBridge::GlfwBridge() : impl_(std::make_unique<Impl>()) {}

GlfwBridge::~GlfwBridge() { Shutdown(); }

bool GlfwBridge::Init(int width, int height, std::string_view title) {
  if (impl_->initialized) {
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
  impl_->window = glfwCreateWindow(width, height, title_str.c_str(), nullptr,
                                   nullptr);
  if (impl_->window == nullptr) {
    LOG(ERROR) << "Failed to create GLFW window";
    glfwTerminate();
    return false;
  }

  auto* user_data = new WindowUserData{this};
  glfwSetWindowUserPointer(impl_->window, user_data);

  glfwSetFramebufferSizeCallback(impl_->window, FramebufferSizeCallback);
  glfwSetKeyCallback(impl_->window, KeyCallback);
  glfwSetMouseButtonCallback(impl_->window, MouseButtonCallback);
  glfwSetCursorPosCallback(impl_->window, CursorPosCallback);
  glfwSetScrollCallback(impl_->window, ScrollCallback);

  impl_->initialized = true;
  LOG(INFO) << "GLFW window created: " << width << "x" << height;
  return true;
}

void GlfwBridge::Shutdown() noexcept {  // NOLINT(bugprone-exception-escape): glog LOG macro may throw
  if (!impl_->initialized) {
    return;
  }

  if (impl_->window != nullptr) {
    auto* user_data =
        static_cast<WindowUserData*>(glfwGetWindowUserPointer(impl_->window));
    delete user_data;

    glfwDestroyWindow(impl_->window);
    impl_->window = nullptr;
  }

  glfwTerminate();
  impl_->initialized = false;
  LOG(INFO) << "GLFW bridge shut down";
}

void GlfwBridge::PollEvents() const {
  if (impl_->initialized) {
    glfwPollEvents();
  }
}

void GlfwBridge::SwapBuffers() {
  if (impl_->window != nullptr) {
    glfwSwapBuffers(impl_->window);
  }
}

void GlfwBridge::MakeContextCurrent() {
  if (impl_->window != nullptr) {
    glfwMakeContextCurrent(impl_->window);
    glfwSwapInterval(1);
  }
}

void GlfwBridge::ReleaseContext() {
  glfwMakeContextCurrent(nullptr);
}

bool GlfwBridge::ShouldClose() const {
  return impl_->window != nullptr && glfwWindowShouldClose(impl_->window) != 0;
}

void GlfwBridge::GetFramebufferSize(int& width, int& height) const {
  if (impl_->window != nullptr) {
    glfwGetFramebufferSize(impl_->window, &width, &height);
  } else {
    width = 0;
    height = 0;
  }
}

void GlfwBridge::GetWindowSize(int& width, int& height) const {
  if (impl_->window != nullptr) {
    glfwGetWindowSize(impl_->window, &width, &height);
  } else {
    width = 0;
    height = 0;
  }
}

GLFWwindow* GlfwBridge::GetNativeHandle() const noexcept {
  return impl_->window;
}

Point GlfwBridge::GetCursorPos() const noexcept {
  if (impl_->window == nullptr) {
    return {.x = 0.0F, .y = 0.0F};
  }
  double xpos = 0.0;
  double ypos = 0.0;
  glfwGetCursorPos(impl_->window, &xpos, &ypos);
  return {.x = static_cast<float>(xpos), .y = static_cast<float>(ypos)};
}

void* GlfwBridge::GetGlContext() const noexcept {
  return static_cast<void*>(impl_->window);
}

void GlfwBridge::SetInputCallback(InputEventCallback callback) noexcept {
  impl_->input_callback = std::move(callback);
}

void GlfwBridge::SetScrollCallback(ScrollEventCallback callback) noexcept {
  impl_->scroll_callback = std::move(callback);
}

void GlfwBridge::SetResizeCallback(ResizeCallback callback) noexcept {
  impl_->resize_callback = std::move(callback);
}

void GlfwBridge::SetMouseMoveCallback(MouseMoveCallback callback) noexcept {
  impl_->mouse_move_callback = std::move(callback);
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
  if (user_data->bridge->impl_->resize_callback) {
    user_data->bridge->impl_->resize_callback(width, height);
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
  if (!bridge->impl_->input_callback) {
    return;
  }
  double cursor_x = 0.0;
  double cursor_y = 0.0;
  glfwGetCursorPos(window, &cursor_x, &cursor_y);
  bridge->impl_->last_cursor_x = cursor_x;
  bridge->impl_->last_cursor_y = cursor_y;
  const auto btn = static_cast<MouseButton>(button);
  const auto act = static_cast<InputAction>(action);
  bridge->impl_->input_callback(btn, act,
                                {.x = static_cast<float>(cursor_x),
                                 .y = static_cast<float>(cursor_y)});
}

void GlfwBridge::CursorPosCallback(GLFWwindow* window, double xpos,
                                   double ypos) {
  auto* user_data =
      static_cast<WindowUserData*>(glfwGetWindowUserPointer(window));
  if (user_data == nullptr || user_data->bridge == nullptr) {
    return;
  }
  user_data->bridge->impl_->last_cursor_x = xpos;
  user_data->bridge->impl_->last_cursor_y = ypos;
  if (user_data->bridge->impl_->mouse_move_callback != nullptr) {
    user_data->bridge->impl_->mouse_move_callback(
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
  if (user_data->bridge->impl_->scroll_callback != nullptr) {
    user_data->bridge->impl_->scroll_callback(xoffset, yoffset);
  }
}

}  // namespace neoflux

#else  // !NEOFLUX_PLATFORM_DESKTOP

namespace neoflux {

GlfwBridge::GlfwBridge() : impl_(std::make_unique<Impl>()) {}

GlfwBridge::~GlfwBridge() = default;

bool GlfwBridge::Init(int /*width*/, int /*height*/,
                      std::string_view /*title*/) {
  return false;
}

void GlfwBridge::Shutdown() noexcept {  // NOLINT(bugprone-exception-escape): glog LOG macro may throw}

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
