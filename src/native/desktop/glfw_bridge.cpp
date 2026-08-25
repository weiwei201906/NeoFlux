// =============================================================================
// NeoFlux - glfw_bridge.cpp
//
// Implementation of GlfwBridge (desktop PlatformBridge via GLFW).
// =============================================================================

#include "neoflux/native/desktop/glfw_bridge.h"

#ifdef NEOFLUX_PLATFORM_DESKTOP

#include <string>
#include <string_view>
#include <utility>

#include <glog/logging.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#ifdef NEOFLUX_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windows.h>
#include <commctrl.h>
#include <unordered_map>
#endif  // NEOFLUX_PLATFORM_WINDOWS

namespace neoflux {

namespace {

struct WindowUserData {
  GlfwBridge* bridge = nullptr;
};

// Convert GLFW mouse button constant to PlatformBridge MouseButton.
MouseButton ConvertMouseButton(int glfw_button) {
  switch (glfw_button) {
    case GLFW_MOUSE_BUTTON_LEFT:
      return MouseButton::kLeft;
    case GLFW_MOUSE_BUTTON_RIGHT:
      return MouseButton::kRight;
    case GLFW_MOUSE_BUTTON_MIDDLE:
      return MouseButton::kMiddle;
    default:
      return MouseButton::kLeft;
  }
}

// Convert GLFW action constant to PlatformBridge InputAction.
// GLFW: RELEASE=0, PRESS=1, REPEAT=2
// PlatformBridge: kPress=0, kRelease=1, kMove=2
InputAction ConvertInputAction(int glfw_action) {
  switch (glfw_action) {
    case GLFW_PRESS:
      return InputAction::kPress;
    case GLFW_RELEASE:
      return InputAction::kRelease;
    case GLFW_REPEAT:
      // Treat repeat as press for compatibility.
      return InputAction::kPress;
    default:
      return InputAction::kRelease;
  }
}

#ifdef NEOFLUX_PLATFORM_WINDOWS
// Forward declaration.
class Win32NativeTextField;

// Global state for subclassing the GLFW window to receive WM_COMMAND from
// child EDIT controls. We subclass once and route EN_CHANGE notifications
// to the correct Win32NativeTextField instance by control ID.
WNDPROC g_original_wndproc = nullptr;
std::unordered_map<int, Win32NativeTextField*> g_text_fields;

// Forward declaration: defined after Win32NativeTextField class.
LRESULT CALLBACK GlfwWndProcSubclass(HWND hwnd, UINT msg, WPARAM wparam,
                                     LPARAM lparam);

// Win32 native single-line text input (EDIT control). Embedded as a child
// window of the GLFW window to get full IME, caret, selection, and clipboard
// support from the OS.
class Win32NativeTextField final : public NativeTextField {
 public:
  explicit Win32NativeTextField(GLFWwindow* window)
      : window_(window), control_id_(next_control_id_++) {
    hwnd_ = glfwGetWin32Window(window);

    // Subclass the GLFW window once to intercept WM_COMMAND.
    if (g_original_wndproc == nullptr) {
      g_original_wndproc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(
          hwnd_, GWLP_WNDPROC,
          reinterpret_cast<LONG_PTR>(GlfwWndProcSubclass)));
    }

    // Create the EDIT control. ES_AUTOHSCROLL enables horizontal scrolling
    // for long text; WS_BORDER gives a thin border consistent with the
    // widget's focus border.
    edit_hwnd_ = CreateWindowExW(
        0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_BORDER | ES_LEFT,
        0, 0, 100, 24, hwnd_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(control_id_)),
        GetModuleHandleW(nullptr), nullptr);

    g_text_fields[control_id_] = this;
  }

  ~Win32NativeTextField() override {
    g_text_fields.erase(control_id_);
    if (font_ != nullptr) {
      DeleteObject(font_);
    }
    if (edit_hwnd_ != nullptr) {
      DestroyWindow(edit_hwnd_);
    }
  }

  // Non-copyable, non-movable: owns native window and font handles.
  Win32NativeTextField(const Win32NativeTextField&) = delete;
  Win32NativeTextField& operator=(const Win32NativeTextField&) = delete;
  Win32NativeTextField(Win32NativeTextField&&) = delete;
  Win32NativeTextField& operator=(Win32NativeTextField&&) = delete;

  void SetText(std::string_view text) override {
    if (edit_hwnd_ == nullptr) {
      return;
    }
    const int wide_len = MultiByteToWideChar(
        CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(wide_len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(),
                        static_cast<int>(text.size()), wide.data(), wide_len);
    SetWindowTextW(edit_hwnd_, wide.c_str());
  }

  [[nodiscard]] std::string GetText() const override {
    if (edit_hwnd_ == nullptr) {
      return {};
    }
    const int len = GetWindowTextLengthW(edit_hwnd_);
    std::wstring wide(static_cast<std::size_t>(len), L'\0');
    GetWindowTextW(edit_hwnd_, wide.data(), len + 1);
    const int utf8_len = WideCharToMultiByte(
        CP_UTF8, 0, wide.c_str(), len, nullptr, 0, nullptr, nullptr);
    std::string utf8(static_cast<std::size_t>(utf8_len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), len, utf8.data(),
                        utf8_len, nullptr, nullptr);
    return utf8;
  }

  void SetPlaceholder(std::string_view text) override {
    if (edit_hwnd_ == nullptr) {
      return;
    }
    const int wide_len = MultiByteToWideChar(
        CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(wide_len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(),
                        static_cast<int>(text.size()), wide.data(), wide_len);
    // EM_SETCUEBANNER shows the cue banner when the edit control is empty.
    SendMessageW(edit_hwnd_, EM_SETCUEBANNER, TRUE,
                 reinterpret_cast<LPARAM>(wide.c_str()));
  }

  void SetPosition(float x, float y) override {
    if (edit_hwnd_ == nullptr) {
      return;
    }
    SetWindowPos(edit_hwnd_, nullptr, static_cast<int>(x),
                 static_cast<int>(y), 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
  }

  void SetSize(float width, float height) override {
    if (edit_hwnd_ == nullptr) {
      return;
    }
    SetWindowPos(edit_hwnd_, nullptr, 0, 0, static_cast<int>(width),
                 static_cast<int>(height),
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
  }

  void SetFontSize(float size) override {
    if (edit_hwnd_ == nullptr) {
      return;
    }
    if (font_ != nullptr) {
      DeleteObject(font_);
    }
    // Create a font. Negative height means character height (not cell height).
    font_ = CreateFontW(
        -static_cast<int>(size), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
    SendMessageW(edit_hwnd_, WM_SETFONT,
                 reinterpret_cast<WPARAM>(font_), TRUE);
  }

  void SetFocus() override {
    if (edit_hwnd_ != nullptr) {
      ::SetFocus(edit_hwnd_);
    }
  }

  void Show() override {
    if (edit_hwnd_ != nullptr) {
      ShowWindow(edit_hwnd_, SW_SHOWNOACTIVATE);
    }
  }

  void Hide() override {
    if (edit_hwnd_ != nullptr) {
      ShowWindow(edit_hwnd_, SW_HIDE);
    }
  }

  void SetOnTextChanged(
      std::function<void(std::string_view)> callback) override {
    on_text_changed_ = std::move(callback);
  }

  // Called from the subclassed window procedure when EN_CHANGE arrives.
  void OnTextChanged() {
    if (on_text_changed_ != nullptr) {
      on_text_changed_(GetText());
    }
  }

 private:
  GLFWwindow* window_;
  HWND hwnd_ = nullptr;
  HWND edit_hwnd_ = nullptr;
  HFONT font_ = nullptr;
  int control_id_ = 0;
  std::function<void(std::string_view)> on_text_changed_{};

  static int next_control_id_;
};

int Win32NativeTextField::next_control_id_ = 1000;

// Subclass procedure for the GLFW window to handle WM_COMMAND from EDIT
// controls. Defined after Win32NativeTextField so it can call OnTextChanged.
LRESULT CALLBACK GlfwWndProcSubclass(HWND hwnd, UINT msg, WPARAM wparam,
                                     LPARAM lparam) {
  if (msg == WM_COMMAND) {
    const int control_id = LOWORD(wparam);
    const int notify_code = HIWORD(wparam);
    auto it = g_text_fields.find(control_id);
    if (it != g_text_fields.end() && notify_code == EN_CHANGE) {
      it->second->OnTextChanged();
    }
  }
  return CallWindowProc(g_original_wndproc, hwnd, msg, wparam, lparam);
}
#endif  // NEOFLUX_PLATFORM_WINDOWS

}  // namespace

GlfwBridge::GlfwBridge() = default;

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

  const std::string title_str(title);
  window_ = glfwCreateWindow(width, height, title_str.c_str(), nullptr,
                             nullptr);
  if (window_ == nullptr) {
    LOG(ERROR) << "Failed to create GLFW window";
    glfwTerminate();
    return false;
  }

  auto* user_data = new WindowUserData{this};
  glfwSetWindowUserPointer(window_, user_data);

  glfwSetFramebufferSizeCallback(window_, FramebufferSizeCallback);
  glfwSetKeyCallback(window_, KeyCallback);
  glfwSetCharCallback(window_, CharCallback);
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

  // Free cached standard cursors.
  if (arrow_cursor_ != nullptr) {
    glfwDestroyCursor(arrow_cursor_);
    arrow_cursor_ = nullptr;
  }
  if (ibeam_cursor_ != nullptr) {
    glfwDestroyCursor(ibeam_cursor_);
    ibeam_cursor_ = nullptr;
  }
  if (hand_cursor_ != nullptr) {
    glfwDestroyCursor(hand_cursor_);
    hand_cursor_ = nullptr;
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

void GlfwBridge::PollEvents() {
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

bool GlfwBridge::ShouldClose() const noexcept {
  return window_ != nullptr && glfwWindowShouldClose(window_) != 0;
}

int GlfwBridge::GetWidth() const noexcept {
  int width = 0;
  int height = 0;
  GetWindowSize(width, height);
  return width;
}

int GlfwBridge::GetHeight() const noexcept {
  int width = 0;
  int height = 0;
  GetWindowSize(width, height);
  return height;
}

void* GlfwBridge::GetNativeHandle() const noexcept {
  return static_cast<void*>(window_);
}

GLFWwindow* GlfwBridge::GetGlfwWindow() const noexcept { return window_; }

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

void GlfwBridge::SetInputCallback(InputEventCallback callback) {
  input_callback_ = std::move(callback);
}

void GlfwBridge::SetKeyCallback(KeyEventCallback callback) {
  key_callback_ = std::move(callback);
}

void GlfwBridge::SetCharCallback(CharEventCallback callback) {
  char_callback_ = std::move(callback);
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

void GlfwBridge::KeyCallback(GLFWwindow* window, int key,
                             int /*scancode*/, int action, int mods) {
  auto* user_data =
      static_cast<WindowUserData*>(glfwGetWindowUserPointer(window));
  if (user_data == nullptr || user_data->bridge == nullptr) {
    return;
  }
  auto* bridge = user_data->bridge;
  if (bridge->key_callback_ == nullptr) {
    return;
  }
  // GLFW key codes match our KeyCode enum values for the subset we define.
  KeyEvent event{};
  event.key = static_cast<KeyCode>(key);
  // GLFW modifier bits match KeyModifiers exactly: shift=0x1, control=0x2,
  // alt=0x4, super=0x8. Mask the lower 4 bits and cast to uint8_t.
  const auto mods_u = static_cast<std::uint32_t>(mods);
  // NOLINTNEXTLINE(bugprone-signed-bitwise) - both operands are unsigned after cast
  event.modifiers = static_cast<std::uint8_t>(mods_u & 0x000FU);
  event.pressed = (action != 0);  // GLFW_RELEASE=0, PRESS=1, REPEAT=2
  bridge->key_callback_(event);
}

void GlfwBridge::CharCallback(GLFWwindow* window, unsigned int codepoint) {
  auto* user_data =
      static_cast<WindowUserData*>(glfwGetWindowUserPointer(window));
  if (user_data == nullptr || user_data->bridge == nullptr) {
    return;
  }
  auto* bridge = user_data->bridge;
  if (bridge->char_callback_ == nullptr) {
    return;
  }
  CharEvent event{};
  event.codepoint = codepoint;
  bridge->char_callback_(event);
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
  double cursor_x = 0.0;
  double cursor_y = 0.0;
  glfwGetCursorPos(window, &cursor_x, &cursor_y);
  bridge->last_cursor_x_ = cursor_x;
  bridge->last_cursor_y_ = cursor_y;
  bridge->input_callback_(
      ConvertMouseButton(button), ConvertInputAction(action),
      {.x = static_cast<float>(cursor_x), .y = static_cast<float>(cursor_y)});
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

void GlfwBridge::SetCursor(CursorType type) {
  if (window_ == nullptr || type == current_cursor_) {
    return;
  }
  current_cursor_ = type;
  GLFWcursor* cursor = nullptr;
  switch (type) {
    case CursorType::kArrow:
      if (arrow_cursor_ == nullptr) {
        arrow_cursor_ = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
      }
      cursor = arrow_cursor_;
      break;
    case CursorType::kIBeam:
      if (ibeam_cursor_ == nullptr) {
        ibeam_cursor_ = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
      }
      cursor = ibeam_cursor_;
      break;
    case CursorType::kHand:
      if (hand_cursor_ == nullptr) {
        hand_cursor_ = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
      }
      cursor = hand_cursor_;
      break;
    case CursorType::kResize:
      if (arrow_cursor_ == nullptr) {
        arrow_cursor_ = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
      }
      cursor = arrow_cursor_;
      break;
  }
  glfwSetCursor(window_, cursor);
}

std::string GlfwBridge::GetClipboardText() const {
  if (window_ == nullptr) {
    return {};
  }
  const char* text = glfwGetClipboardString(window_);
  if (text == nullptr) {
    return {};
  }
  return std::string(text);
}

void GlfwBridge::SetClipboardText(std::string_view text) {
  if (window_ == nullptr) {
    return;
  }
  std::string buffer(text);
  glfwSetClipboardString(window_, buffer.c_str());
}

std::unique_ptr<NativeTextField> GlfwBridge::CreateNativeTextField() {
#ifdef NEOFLUX_PLATFORM_WINDOWS
  if (window_ == nullptr) {
    return nullptr;
  }
  return std::make_unique<Win32NativeTextField>(window_);
#else
  // Linux (GTK) and macOS (Cocoa) native text fields are not yet implemented.
  // Fall back to nullptr so TextField uses its tgfx-rendered path.
  return nullptr;
#endif  // NEOFLUX_PLATFORM_WINDOWS
}

}  // namespace neoflux

#else  // !NEOFLUX_PLATFORM_DESKTOP

namespace neoflux {

GlfwBridge::GlfwBridge() = default;
GlfwBridge::~GlfwBridge() = default;

bool GlfwBridge::Init(int /*width*/, int /*height*/,
                      std::string_view /*title*/) {
  return false;
}
void GlfwBridge::Shutdown() {}
void GlfwBridge::PollEvents() {}
void GlfwBridge::SwapBuffers() {}
void GlfwBridge::MakeContextCurrent() {}
void GlfwBridge::ReleaseContext() {}
bool GlfwBridge::ShouldClose() const noexcept { return false; }
int GlfwBridge::GetWidth() const noexcept { return 0; }
int GlfwBridge::GetHeight() const noexcept { return 0; }
void* GlfwBridge::GetNativeHandle() const noexcept { return nullptr; }
GLFWwindow* GlfwBridge::GetGlfwWindow() const noexcept { return nullptr; }
void GlfwBridge::GetFramebufferSize(int& width, int& height) const {
  width = 0; height = 0;
}
void GlfwBridge::GetWindowSize(int& width, int& height) const {
  width = 0; height = 0;
}
Point GlfwBridge::GetCursorPos() const noexcept {
  return {.x = 0.0F, .y = 0.0F};
}
void* GlfwBridge::GetGlContext() const noexcept { return nullptr; }
void GlfwBridge::SetInputCallback(InputEventCallback /*callback*/) {}
void GlfwBridge::SetScrollCallback(ScrollEventCallback /*callback*/) noexcept {}
void GlfwBridge::SetResizeCallback(ResizeCallback /*callback*/) noexcept {}
void GlfwBridge::SetMouseMoveCallback(MouseMoveCallback /*callback*/) noexcept {}
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
void GlfwBridge::SetCursor(CursorType /*type*/) {}
std::string GlfwBridge::GetClipboardText() const { return {}; }
void GlfwBridge::SetClipboardText(std::string_view /*text*/) {}

}  // namespace neoflux

#endif  // NEOFLUX_PLATFORM_DESKTOP
