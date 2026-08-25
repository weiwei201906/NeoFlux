// =============================================================================
// NeoFlux - application.h
//
// Top-level Application class. All method implementations are in
// application.cpp.
// =============================================================================

#ifndef NEOFLUX_APP_APPLICATION_H_
#define NEOFLUX_APP_APPLICATION_H_

#include <atomic>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "neoflux/app/event_loop.h"
#include "neoflux/core/noncopyable.h"
#include "neoflux/core/types.h"
#include "neoflux/render/render_context.h"
#include "neoflux/render/render_layer.h"
#include "neoflux/widget/widget.h"

// Desktop-only: GLFW bridge for window creation and input. Mobile platforms
// use the platform bridge (EGL/EAGL) provided by RenderLayer directly.
#ifdef NEOFLUX_PLATFORM_DESKTOP
#include "neoflux/native/desktop/glfw_bridge.h"
#endif

namespace neoflux {

// Top-level application object.
class Application : public NonCopyable {
 public:
  Application();
  ~Application();

  // Initializes the application.
  //
  // On desktop, `platform_surface` is ignored and a GLFW window is created.
  // On mobile, `platform_surface` must be the native surface handle for
  // tgfx to render into.
  bool Init(int argc, char** argv, int window_width = 800,
            int window_height = 600,
            std::string_view window_title = "NeoFlux",
            void* platform_surface = nullptr);

  // Runs the application event loop.
  void Run();

  // Requests the application to shut down.
  void Stop();

  // Pushes a route onto the navigation stack.
  void PushRoute(std::string_view route_name);

  // Pops the top route from the navigation stack.
  void PopRoute();

  // Returns the current root widget (top of the navigation stack).
  [[nodiscard]] Widget* GetRootWidget() const noexcept;

  // Returns the render layer.
  [[nodiscard]] RenderLayer& GetRenderLayer() noexcept;

  // Returns the event loop.
  [[nodiscard]] EventLoop& GetEventLoop() noexcept;

  // Returns the window width.
  [[nodiscard]] int GetWindowWidth() const noexcept;

  // Returns the window height.
  [[nodiscard]] int GetWindowHeight() const noexcept;

  // Marks the next frame as dirty, forcing a build/layout/paint cycle.
  // Thread-safe; wakes the event loop if it is idle.
  void MarkFrameDirty() noexcept;

  // Sets the directory to scan for font files (.ttf, .otf, .ttc). Must be
  // called before Init(). Relative paths are resolved from the working
  // directory, with upward fallback (../, ../../) for build subdirectories.
  // Default: "fonts".
  void SetFontDir(std::string_view dir) noexcept;

  // Returns the configured font directory.
  [[nodiscard]] std::string_view GetFontDir() const noexcept;

 private:
  void OnFrame();
  // Builds all dirty widgets. Returns true if at least one widget was
  // rebuilt (indicating layout/paint should run).
  bool BuildDirtyWidgets();
  void LayoutWidgetTree() const;
  void PaintAndSubmit();
  // Recursively builds dirty widgets. Returns true if any widget in the
  // subtree was rebuilt. Static: does not access any member state.
  static bool BuildWidgetRecursive(Widget& widget, BuildContext& context);
  static void PaintWidgetRecursive(Widget& widget, RenderContext& context);
  void DispatchPointerEvent(MouseButton button, InputAction action,
                            const Point& pos);
  void DispatchPointerMove(const Point& pos);
  void DispatchScrollEvent(double xoffset, double yoffset);
  void DispatchKeyEvent(const KeyEvent& event);
  void DispatchCharEvent(const CharEvent& event);
  // Sets the keyboard focus to the given widget (pass nullptr to clear focus).
  void SetFocus(std::shared_ptr<Widget> widget);
  // Returns the widget that currently has keyboard focus.
  [[nodiscard]] std::shared_ptr<Widget> GetFocusedWidget() const;
  // Invalidates the hit-test cache. Called after layout or tree changes.
  void InvalidateHitCache() noexcept;

  EventLoop event_loop_{};
  std::unique_ptr<RenderLayer> render_layer_ = nullptr;
  std::vector<std::shared_ptr<Widget>> navigation_stack_{};
  RenderContext render_context_{};
  // Widget that received pointer-down. Stored as weak_ptr to avoid dangling
  // references if the widget tree is rebuilt between press and release.
  std::weak_ptr<Widget> pressed_widget_{};
  // Widget currently under the cursor (for enter/exit detection).
  std::weak_ptr<Widget> hovered_widget_{};
  // Cached hit-test result. Invalidated on layout/tree changes; reused for
  // high-frequency pointer-move events to avoid full-tree traversal.
  std::weak_ptr<Widget> hit_cache_{};
  bool hit_cache_valid_ = false;
  // Widget that currently has keyboard focus. Stored as weak_ptr to avoid
  // dangling references if the widget tree is rebuilt.
  std::weak_ptr<Widget> focused_widget_{};
  std::uint16_t window_width_ = 800;
  std::uint16_t window_height_ = 600;
  // Directory to scan for font files. Configured via SetFontDir() before
  // Init(); default "fonts".
  std::string font_dir_{"fonts"};
  bool initialized_ = false;
  // Set when the widget tree or window state changes; cleared after a full
  // frame is processed. When false, OnFrame skips build/layout/paint.
  std::atomic<bool> frame_dirty_{true};
};

}  // namespace neoflux

#endif  // NEOFLUX_APP_APPLICATION_H_
