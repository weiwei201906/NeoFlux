// =============================================================================
// NeoFlux - widget.cpp
//
// Implementation of Widget, BuildContext, StatefulWidget, StatelessWidget.
// Layout is delegated to the Taitank flexbox engine. Each Widget owns an
// opaque Taitank node; the widget tree and Taitank node tree are kept in
// sync via AddChild / ClearChildren.
// =============================================================================

#include "neoflux/widget/widget.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <string_view>

#include <glog/logging.h>

#include "taitank.h"

#include "neoflux/app/application.h"
#include "neoflux/core/types.h"
#include "neoflux/render/render_context.h"

namespace neoflux {

namespace {

// Taitank measure function trampoline. Taitank calls this during layout to
// query the intrinsic size of a leaf node. The node's context pointer holds
// the owning Widget*; we forward to Widget::OnMeasure().
taitank::TaitankSize MeasureTrampoline(taitank::TaitankNodeRef node,
                                       float width,
                                       taitank::MeasureMode width_mode,
                                       float height,
                                       taitank::MeasureMode height_mode,
                                       void* /*layout_context*/) {
  auto* widget = static_cast<Widget*>(taitank::GetContext(node));
  if (widget == nullptr) {
    return {.width = 0.0F, .height = 0.0F};
  }
  const Size size = widget->OnMeasure(width, static_cast<int>(width_mode),
                                      height, static_cast<int>(height_mode));
  return {.width = size.width, .height = size.height};
}

}  // namespace

// ---------------------------------------------------------------------------
// BuildContext
// ---------------------------------------------------------------------------

BuildContext::BuildContext(Application* app) : app_(app) {}

Application* BuildContext::GetApplication() const noexcept { return app_; }

void BuildContext::PushRoute(std::string_view route_name) {
  if (app_ != nullptr) {
    app_->PushRoute(route_name);
  }
}

void BuildContext::PopRoute() {
  if (app_ != nullptr) {
    app_->PopRoute();
  }
}

// ---------------------------------------------------------------------------
// Widget
// ---------------------------------------------------------------------------

Widget::Widget() : taitank_node_(taitank::NodeCreate()) {
  if (taitank_node_ != nullptr) {
    taitank::SetContext(taitank_node_, this);
    // Default to column layout (vertical stacking), matching the typical
    // UI pattern. Containers may override with SetFlexDirection(kRow).
    // Taitank's factory default is FLEX_DIRECTION_ROW (enum value 0),
    // which would lay children horizontally 鈥?wrong for most widgets.
    taitank::SetFlexDirection(taitank_node_, taitank::FLEX_DIRECTION_COLUMN);
    taitank::SetAlignItems(taitank_node_, taitank::FLEX_ALIGN_STRETCH);
    // Note: measure function is NOT set here. Only leaf widgets (Text,
    // Button, etc.) set a measure function. Taitank forbids children on
    // nodes with a measure function, so layout containers must not have one.
  }
}

Widget::~Widget() {
  if (taitank_node_ != nullptr) {
    taitank::NodeFree(taitank_node_);
    taitank_node_ = nullptr;
  }
}

std::shared_ptr<Widget> Widget::Build(BuildContext& /*context*/) {
  return nullptr;
}

bool Widget::OnPointerDown(const Point& /*local_pos*/) { return false; }

void Widget::OnPointerUp(const Point& /*local_pos*/) {}

bool Widget::OnPointerScroll(const Point& /*local_pos*/, double /*xoffset*/,
                             double /*yoffset*/) {
  return false;
}

bool Widget::OnPointerMove(const Point& /*local_pos*/) { return false; }

void Widget::OnPointerEnter() {}

void Widget::OnPointerExit() {}

std::shared_ptr<Widget> Widget::HitTest(
    const Point& parent_pos) {
  // parent_pos is relative to this widget's parent. bounds_ is also relative
  // to the parent, so we can compare directly.
  if (parent_pos.x < bounds_.x || parent_pos.y < bounds_.y ||
      parent_pos.x >= bounds_.x + bounds_.width ||
      parent_pos.y >= bounds_.y + bounds_.height) {
    VLOG(2) << "HitTest: " << GetWidgetName() << " bounds ["
            << bounds_.x << "," << bounds_.y << " " << bounds_.width << "x"
            << bounds_.height << "] MISS point (" << parent_pos.x << ","
            << parent_pos.y << ")";
    return nullptr;
  }
  VLOG(2) << "HitTest: " << GetWidgetName() << " bounds ["
          << bounds_.x << "," << bounds_.y << " " << bounds_.width << "x"
          << bounds_.height << "] HIT point (" << parent_pos.x << ","
          << parent_pos.y << "), children=" << children_.size();
  // Convert to this widget's local coordinates before recursing into children,
  // because children's bounds_ are relative to this widget.
  const Point local_pos{.x = parent_pos.x - bounds_.x,
                        .y = parent_pos.y - bounds_.y,};
  // Test children in reverse order (top-most / last painted first).
  for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
    if (*it == nullptr) {
      continue;
    }
    std::shared_ptr<Widget> hit = (*it)->HitTest(local_pos);
    if (hit != nullptr) {
      return hit;
    }
  }
  return shared_from_this();
}

void Widget::PerformLayout(float width, float height) {
  if (taitank_node_ == nullptr) {
    return;
  }
  SyncTaitankChildren();
  // The root widget (no parent) must fill the entire viewport. Taitank
  // nodes default to NaN (auto) sizing, which would make the root shrink
  // to its content size instead of filling the window. Force the root's
  // dimensions to the viewport size before layout.
  if (parent_ == nullptr) {
    taitank::SetWidth(taitank_node_, width);
    taitank::SetHeight(taitank_node_, height);
  }
  taitank::DoLayout(taitank_node_, width, height, taitank::DIRECTION_LTR);
  ReadLayoutRecursive();
}

void Widget::AddChild(std::shared_ptr<Widget> child) {
  if (child == nullptr) {
    return;
  }
  child->SetParent(this);
  children_.push_back(std::move(child));
  if (taitank_node_ != nullptr) {
    const auto& added = children_.back();
    taitank::InsertChild(taitank_node_, added->GetTaitankNode(),
                         static_cast<uint32_t>(children_.size() - 1));
  }
}

void Widget::ClearChildren() {
  if (taitank_node_ != nullptr) {
    for (auto& child : children_) {
      if (child != nullptr) {
        taitank::RemoveChild(taitank_node_, child->GetTaitankNode());
      }
    }
  }
  for (auto& child : children_) {
    if (child != nullptr) {
      child->SetParent(nullptr);
    }
  }
  children_.clear();
}

const std::vector<std::shared_ptr<Widget>>& Widget::GetChildren()
    const noexcept {
  return children_;
}

std::size_t Widget::GetChildCount() const noexcept { return children_.size(); }

void Widget::SetParent(Widget* parent) noexcept { parent_ = parent; }

Widget* Widget::GetParent() const noexcept { return parent_; }

void Widget::SetBounds(const Rect& bounds) noexcept { bounds_ = bounds; }

const Rect& Widget::GetBounds() const noexcept { return bounds_; }

Point Widget::GetGlobalPosition() const noexcept {
  Point pos{.x = bounds_.x, .y = bounds_.y};
  const Widget* p = parent_;
  while (p != nullptr) {
    pos.x += p->bounds_.x;
    pos.y += p->bounds_.y;
    p = p->parent_;
  }
  return pos;
}

void Widget::SetDesiredSize(const Size& size) noexcept {
  desired_size_ = size;
}

const Size& Widget::GetDesiredSize() const noexcept { return desired_size_; }

void Widget::MarkNeedsBuild() noexcept { needs_build_ = true; }

bool Widget::NeedsBuild() const noexcept { return needs_build_; }

void Widget::ClearNeedsBuild() noexcept { needs_build_ = false; }

taitank::TaitankNode* Widget::GetTaitankNode() const noexcept {
  return taitank_node_;
}

void Widget::SetState(WidgetState new_state) {
  if (new_state == state_) {
    return;
  }
  const WidgetState old = state_;
  state_ = new_state;
  OnStateChanged(old, new_state);
}

WidgetState Widget::GetState() const noexcept { return state_; }

void Widget::OnStateChanged(WidgetState /*from*/, WidgetState /*to*/) {
  // Default: no-op. Subclasses override to launch coroutines, mark dirty
  // frames, or update visual properties on state transitions.
}

void Widget::SyncTaitankChildren() {
  if (taitank_node_ == nullptr) {
    return;
  }
  // Remove all existing Taitank children, then re-insert to match the
  // widget children vector. This handles rebuilds where children change.
  while (taitank::ChildCount(taitank_node_) > 0) {
    taitank::TaitankNodeRef first = taitank::GetChild(taitank_node_, 0);
    if (first == nullptr) {
      break;
    }
    taitank::RemoveChild(taitank_node_, first);
  }
  for (std::size_t i = 0; i < children_.size(); ++i) {
    if (children_[i] != nullptr) {
      auto* child_node = children_[i]->GetTaitankNode();
      if (child_node != nullptr) {
        taitank::InsertChild(taitank_node_, child_node,
                             static_cast<uint32_t>(i));
      }
    }
  }
}

void Widget::ReadLayoutRecursive() {
  if (taitank_node_ != nullptr) {
    bounds_.x = taitank::GetLeft(taitank_node_);
    bounds_.y = taitank::GetTop(taitank_node_);
    bounds_.width = taitank::GetWidth(taitank_node_);
    bounds_.height = taitank::GetHeight(taitank_node_);
    desired_size_ = {.width = bounds_.width, .height = bounds_.height};
  }
  for (auto& child : children_) {
    if (child != nullptr) {
      child->ReadLayoutRecursive();
    }
  }
}

void Widget::EnableMeasureFunction() {
  if (taitank_node_ != nullptr) {
    taitank::SetMeasureFunction(taitank_node_, MeasureTrampoline);
  }
}

void Widget::PaintChildren(RenderContext& context) {
  for (const auto& child : children_) {
    if (child == nullptr) {
      continue;
    }
    context.Save();
    context.Translate(child->GetBounds().x, child->GetBounds().y);
    child->Paint(context);
    context.Restore();
  }
}

// ---------------------------------------------------------------------------
// StatefulWidget
// ---------------------------------------------------------------------------

StatefulWidget::~StatefulWidget() {
  if (state_ != nullptr) {
    state_->Dispose();
  }
}

std::string_view StatefulWidget::GetWidgetName() const noexcept {
  return "StatefulWidget";
}

void StatefulWidget::Paint(RenderContext& context) { PaintChildren(context); }

Size StatefulWidget::OnMeasure(float /*width*/, int /*width_mode*/,
                                float /*height*/, int /*height_mode*/) {
  return {.width = 0.0F, .height = 0.0F};
}

std::shared_ptr<Widget> StatefulWidget::Build(BuildContext& context) {
  if (state_ == nullptr) {
    state_ = CreateState();
    if (state_ != nullptr) {
      state_->SetWidget(this);
      state_->InitState();
    }
  }
  if (state_ != nullptr) {
    state_->SetContext(&context);
    return state_->Build(context);
  }
  return nullptr;
}

State<StatefulWidget>* StatefulWidget::GetState() const noexcept {
  return state_.get();
}

// ---------------------------------------------------------------------------
// StatelessWidget
// ---------------------------------------------------------------------------

std::string_view StatelessWidget::GetWidgetName() const noexcept {
  return "StatelessWidget";
}

// ---------------------------------------------------------------------------
// State<W> template implementations (explicitly instantiated below)
// ---------------------------------------------------------------------------

template <typename W>
W* State<W>::GetWidget() const noexcept {
  return widget_;
}

template <typename W>
BuildContext* State<W>::GetContext() const noexcept {
  return context_;
}

template <typename W>
void State<W>::SetState(std::function<void()> callback) {
  if (callback) {
    callback();
  }
  if (widget_ != nullptr) {
    widget_->MarkNeedsBuild();
  }
}

template <typename W>
void State<W>::InitState() {}

template <typename W>
void State<W>::Dispose() {}

template <typename W>
void State<W>::SetWidget(W* widget) noexcept {
  widget_ = widget;
}

template <typename W>
void State<W>::SetContext(BuildContext* context) noexcept {
  context_ = context;
}

// Explicit instantiation: all user states inherit State<StatefulWidget>.
template class State<StatefulWidget>;

}  // namespace neoflux
