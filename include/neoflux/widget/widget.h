// =============================================================================
// NeoFlux - widget.h
//
// Base Widget class and BuildContext for the Flutter-like widget tree.
// Layout is delegated to the Taitank flexbox engine: each Widget owns an
// opaque Taitank node, and the widget tree mirrors the Taitank node tree.
// All method implementations (including State<W> template methods) are in
// widget.cpp with explicit instantiation for State<StatefulWidget>.
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

// Opaque Taitank layout node (defined in taitank_node.h).
namespace taitank { struct TaitankNode; }

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
// A Widget is a node in the UI tree. It holds an opaque Taitank flex node
// that participates in layout, an optional list of children, and its
// computed layout rectangle. Subclasses override Build() for composition,
// OnMeasure() for intrinsic sizing, and Paint() for rendering.
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

  // Measures the intrinsic size of this widget given Taitank constraints.
  //
  // Leaf widgets with intrinsic size (Text, Button) override this to report
  // their desired dimensions. Called by the Taitank engine during layout via
  // the measure function.
  [[nodiscard]] virtual Size OnMeasure(float width, int width_mode,
                                       float height, int height_mode) = 0;

  // Paints this widget and its children onto the given render context.
  virtual void Paint(RenderContext& context) = 0;

  // Handles a pointer down event at the given local coordinates.
  //
  // Returns true if the event was consumed by this widget. The default
  // implementation returns false. Subclasses that respond to touch/click
  // (e.g. Button) override this.
  virtual bool OnPointerDown(const Point& local_pos);

  // Handles a pointer up event at the given local coordinates.
  virtual void OnPointerUp(const Point& local_pos);

  // Handles a mouse scroll event at the given local coordinates.
  // xoffset/yoffset are in normalized units (positive = up/right).
  // Returns true if the event was consumed.
  virtual bool OnPointerScroll(const Point& local_pos, double xoffset,
                               double yoffset);

  // Performs a hit test at the given global coordinates.
  //
  // Returns the deepest widget that contains the point, or nullptr if no
  // widget is hit. Children are tested in reverse order (top-most first).
  // Performs a hit test at the given parent-relative coordinates.
  // Returns the deepest widget that contains the point, or nullptr if no
  // widget is hit. Children are tested in reverse order (top-most first).
  // For the root widget, the coordinate is the window coordinate.
  [[nodiscard]] std::shared_ptr<Widget> HitTest(const Point& parent_pos);

  // Returns the widget's global position (sum of all ancestor bounds offsets).
  [[nodiscard]] Point GetGlobalPosition() const noexcept;

  // Performs Taitank layout rooted at this widget with the given available
  // size, then recursively copies computed bounds back into the widget tree.
  void PerformLayout(float width, float height);

  // Adds a child widget. Also inserts the child's Taitank node into this
  // widget's Taitank node to keep the two trees in sync.
  void AddChild(std::shared_ptr<Widget> child);

  // Removes all children. Also clears them from the Taitank node.
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

  // Returns the opaque Taitank node handle (for subclasses to set styles).
  [[nodiscard]] taitank::TaitankNode* GetTaitankNode() const noexcept;

 protected:
  // Paints all children with appropriate translation.
  void PaintChildren(RenderContext& context);

  // Recomputes the Taitank node tree to match the widget children.
  // Called after build phase when children may have changed.
  void SyncTaitankChildren();

  // Recursively reads computed bounds from Taitank nodes into widgets.
  virtual void ReadLayoutRecursive();

  // Enables the Taitank measure function on this widget's node. Must be
  // called by leaf widgets (Text, Button, etc.) in their constructors.
  // Nodes with a measure function cannot have children (Taitank constraint).
  void EnableMeasureFunction();

  // Derived widgets may read their computed bounds directly.
  Rect bounds_{};
  Size desired_size_{};

 private:
  std::vector<std::shared_ptr<Widget>> children_{};
  Widget* parent_ = nullptr;
  bool needs_build_ = true;
  taitank::TaitankNode* taitank_node_ = nullptr;  // Opaque Taitank node.
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
  void SetState(std::function<void()> callback);

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

  // StatefulWidget delegates painting to its built child.
  void Paint(RenderContext& context) override;

  // StatefulWidget has no intrinsic size; delegates to layout.
  [[nodiscard]] Size OnMeasure(float width, int width_mode, float height,
                               int height_mode) override;

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
