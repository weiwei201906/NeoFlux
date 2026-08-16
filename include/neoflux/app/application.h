// =============================================================================
// NeoFlux - application.h
//
// Top-level Application class. All method implementations are in
// application.cpp.
// =============================================================================

#ifndef NEOFLUX_APP_APPLICATION_H_
#define NEOFLUX_APP_APPLICATION_H_

#include <memory>
#include <string_view>
#include <vector>

#include "neoflux/app/event_loop.h"
#include "neoflux/core/noncopyable.h"
#include "neoflux/core/types.h"
#include "neoflux/render/render_context.h"
#include "neoflux/render/render_layer.h"
#include "neoflux/widget/widget.h"

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

 private:
  void OnFrame();
  void BuildDirtyWidgets();
  void LayoutWidgetTree();
  void PaintAndSubmit();
  void BuildWidgetRecursive(Widget& widget, BuildContext& context);
  Size LayoutWidgetRecursive(Widget& widget,
                             const LayoutConstraints& constraints);
  void PaintWidgetRecursive(Widget& widget, RenderContext& context);

  EventLoop event_loop_;
  std::unique_ptr<RenderLayer> render_layer_;
  std::vector<std::shared_ptr<Widget>> navigation_stack_;
  RenderContext render_context_;
  int window_width_;
  int window_height_;
  bool initialized_;
};

}  // namespace neoflux

#endif  // NEOFLUX_APP_APPLICATION_H_
