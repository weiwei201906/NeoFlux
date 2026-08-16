// =============================================================================
// NeoFlux - widget.cpp
//
// Implementation of Widget, BuildContext, StatefulWidget, StatelessWidget.
// All non-template methods moved from widget.h.
// =============================================================================

#include "neoflux/widget/widget.h"

#include <algorithm>

#include <glog/logging.h>

#include "neoflux/app/application.h"
#include "neoflux/render/render_context.h"

namespace neoflux {

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

Widget::Widget() = default;

Widget::~Widget() = default;

std::shared_ptr<Widget> Widget::Build(BuildContext& /*context*/) {
  return nullptr;
}

Size Widget::Layout(const LayoutConstraints& constraints) {
  float max_width = constraints.min_width;
  float max_height = constraints.min_height;

  for (auto& child : children_) {
    if (child != nullptr) {
      const Size child_size = LayoutChild(*child, constraints);
      max_width = std::max(max_width, child_size.width);
      max_height = std::max(max_height, child_size.height);
    }
  }

  max_width =
      std::clamp(max_width, constraints.min_width, constraints.max_width);
  max_height =
      std::clamp(max_height, constraints.min_height, constraints.max_height);

  bounds_.width = max_width;
  bounds_.height = max_height;
  desired_size_ = {.width = max_width, .height = max_height};
  return desired_size_;
}

void Widget::Paint(RenderContext& context) { PaintChildren(context); }

void Widget::AddChild(std::shared_ptr<Widget> child) {
  if (child != nullptr) {
    child->SetParent(this);
    children_.push_back(std::move(child));
  }
}

void Widget::ClearChildren() {
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

void Widget::SetDesiredSize(const Size& size) noexcept {
  desired_size_ = size;
}

const Size& Widget::GetDesiredSize() const noexcept { return desired_size_; }

void Widget::MarkNeedsBuild() noexcept { needs_build_ = true; }

bool Widget::NeedsBuild() const noexcept { return needs_build_; }

void Widget::ClearNeedsBuild() noexcept { needs_build_ = false; }

Size Widget::LayoutChild(Widget& child,
                         const LayoutConstraints& constraints) {
  const Size size = child.Layout(constraints);
  child.SetBounds({.x = 0.0F, .y = 0.0F, .width = size.width, .height = size.height});
  return size;
}

void Widget::PaintChildren(RenderContext& context) {
  for (const auto& child : children_) {
    if (child != nullptr) {
      context.Save();
      context.Translate(child->GetBounds().x, child->GetBounds().y);
      child->Paint(context);
      context.Restore();
    }
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
void State<W>::SetState(std::function<void()> fn) {
  if (fn) {
    fn();
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
