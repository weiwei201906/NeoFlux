// =============================================================================
// NeoFlux - widget.h
//
// Base Widget class and BuildContext for the Flutter-like widget tree.
// All non-template method implementations are in widget.cpp.
//
// Note: State<W> is a class template and must remain header-only (C++
// template instantiation requirement).
// =============================================================================

#ifndef NEOFLUX_WIDGET_WIDGET_H_
#define NEOFLUX_WIDGET_WIDGET_H_

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "neoflux/core/noncopyable.h"
#include "neoflux/core/types.h"

namespace neoflux {

// Forward declarations.
class Application;
class RenderContext;

// Build context passed to widget build() methods.
//
// Provides access to the owning Application and convenience methods for
// navigation. A BuildContext is only valid during the build phase of a
// frame.
class BuildContext {
 public:
  explicit BuildContext(Application* app);

  // Returns the owning Application instance.
  [[nodiscard]] Application* GetApplication() const noexcept;

  // Pushes a new route onto the navigation stack.
  void PushRoute(std::string_view route_name);

  // Pops the top route from the navigation stack.
  void PopRoute();

 private:
  Application* app_;  // Not owned.
};

// Base class for all widgets in the NeoFlux framework.
//
// A Widget is a node in the UI tree. It holds an optional list of children,
// its computed layout rectangle, and provides virtual hooks for building,
// layout, and painting.
//
// Thread safety: Widget objects are owned and mutated exclusively by the
//                 Application (UI) thread.
class Widget : public std::enable_shared_from_this<Widget> {
 public:
  Widget();
  virtual ~Widget();

  // Non-copyable, non-movable (widget identity is stable in the tree).
  Widget(const Widget&) = delete;
  Widget& operator=(const Widget&) = delete;
  Widget(Widget&&) = delete;
  Widget& operator=(Widget&&) = delete;

  // Returns a human-readable name for this widget type (for debugging).
  [[nodiscard]] virtual std::string_view GetWidgetName() const noexcept = 0;

  // Builds the widget subtree.
  //
  // Called during the build phase. The default implementation returns
  // nullptr (leaf widget). Subclasses override to produce children.
  [[nodiscard]] virtual std::shared_ptr<Widget> Build(BuildContext& context);

  // Performs layout for this widget and its children.
  virtual Size Layout(const LayoutConstraints& constraints);

  // Paints this widget and its children onto the given render context.
  virtual void Paint(RenderContext& context);

  // Adds a child widget.
  void AddChild(std::shared_ptr<Widget> child);

  // Removes all children.
  void ClearChildren();

  // Returns a read-only view of the children.
  [[nodiscard]] const std::vector<std::shared_ptr<Widget>>& GetChildren()
      const noexcept;

  // Returns the number of children.
  [[nodiscard]] std::size_t GetChildCount() const noexcept;

  // Sets the parent widget (called by AddChild).
  void SetParent(Widget* parent) noexcept;

  // Returns the parent widget, or nullptr if this is the root.
  [[nodiscard]] Widget* GetParent() const noexcept;

  // Sets the computed layout bounds (position + size).
  void SetBounds(const Rect& bounds) noexcept;

  // Returns the computed layout bounds.
  [[nodiscard]] const Rect& GetBounds() const noexcept;

  // Sets the widget's desired size (used by layout).
  void SetDesiredSize(const Size& size) noexcept;

  // Returns the widget's desired size.
  [[nodiscard]] const Size& GetDesiredSize() const noexcept;

  // Marks this widget as needing a rebuild.
  void MarkNeedsBuild() noexcept;

  // Returns whether this widget needs a rebuild.
  [[nodiscard]] bool NeedsBuild() const noexcept;

  // Clears the needs-build flag.
  void ClearNeedsBuild() noexcept;

 protected:
  // Lays out a single child with the given constraints.
  Size LayoutChild(Widget& child, const LayoutConstraints& constraints);

  // Paints all children with appropriate translation.
  void PaintChildren(RenderContext& context);

 private:
  std::vector<std::shared_ptr<Widget>> children_;
  Widget* parent_;
  Rect bounds_;
  Size desired_size_;
  bool needs_build_;
};

// Mutable state for a StatefulWidget.
//
// Holds data that can change over the lifetime of the widget. Call
// SetState() to mutate state and schedule a rebuild of the widget subtree.
//
// Note: This is a class template. Method implementations are in widget.cpp
// with explicit instantiation for State<StatefulWidget>. All user-defined
// states must inherit from State<StatefulWidget>.
//
// Template parameter W is the widget type that owns this state.
template <typename W>
class State {
 public:
  virtual ~State() = default;

  // Returns the widget that owns this state.
  [[nodiscard]] W* GetWidget() const noexcept;

  // Returns the build context.
  [[nodiscard]] BuildContext* GetContext() const noexcept;

  // Schedules a rebuild of the widget subtree.
  void SetState(std::function<void()> fn);

  // Called when this state is first created.
  virtual void InitState();

  // Called when this state is disposed.
  virtual void Dispose();

  // Builds the widget subtree for the current state.
  [[nodiscard]] virtual std::shared_ptr<Widget> Build(
      BuildContext& context) = 0;

  // Internal: sets the owning widget. Called by the framework.
  void SetWidget(W* widget) noexcept;

  // Internal: sets the build context. Called by the framework.
  void SetContext(BuildContext* context) noexcept;

 private:
  W* widget_ = nullptr;
  BuildContext* context_ = nullptr;
};

// Base class for stateful widgets.
//
// A StatefulWidget has mutable state that persists across rebuilds. The
// widget owns its State instance (created lazily on first build).
class StatefulWidget : public Widget {
 public:
  ~StatefulWidget() override;

  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;

  // Creates the State object for this widget.
  [[nodiscard]] virtual std::unique_ptr<State<StatefulWidget>> CreateState() = 0;

  // Builds the widget subtree using the associated State.
  [[nodiscard]] std::shared_ptr<Widget> Build(BuildContext& context) override;

  // Returns the associated state, or nullptr if not yet created.
  [[nodiscard]] State<StatefulWidget>* GetState() const noexcept;

 private:
  std::unique_ptr<State<StatefulWidget>> state_;
};

// Base class for stateless widgets.
class StatelessWidget : public Widget {
 public:
  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_WIDGET_H_
