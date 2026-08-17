// =============================================================================
// NeoFlux - drag_demo
//
// Demonstrates the Draggable widget with the widget state machine and
// coroutine integration. A colored box can be dragged around; a status
// label shows the current state (Idle/Hovering/Dragging) and offset.
//
// Also demonstrates the "state machine as condition lock" pattern: a
// long-press coroutine is launched on pointer-down, but it checks the
// widget state before firing. If the user releases before the timeout,
// the coroutine observes the state change and silently returns.
// =============================================================================

#include <chrono>
#include <memory>
#include <string>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "neoflux/neoflux.h"

using namespace neoflux;  // NOLINT(google-build-using-namespace)

namespace {

// A draggable box that reports its state via a callback. The long-press
// coroutine uses weak_ptr + state check as an implicit cancellation
// mechanism: if the widget is released before 500ms, the coroutine sees
// the state change and returns without firing.
class DragBox : public Draggable {
 public:
  using StateCallback = std::function<void(WidgetState, const Point&)>;

  DragBox(Application* app, StateCallback on_state)
      : on_state_(std::move(on_state)), app_(app) {}

  bool OnPointerDown(const Point& local_pos) override {
    Draggable::OnPointerDown(local_pos);
    // Launch a long-press detection coroutine. It uses a weak_ptr to this
    // widget and checks the state before firing. If the pointer is released
    // before 500ms, the coroutine observes kIdle and returns silently.
    auto weak_self = std::weak_ptr<DragBox>(
        std::static_pointer_cast<DragBox>(shared_from_this()));
    if (app_ != nullptr) {
      app_->GetEventLoop().Schedule(LongPressCoroutine(weak_self));
    }
    return true;
  }

  void OnPointerUp(const Point& local_pos) override {
    Draggable::OnPointerUp(local_pos);
    long_press_fired_ = false;
  }

  void SetLongPressFired(bool fired) noexcept { long_press_fired_ = fired; }
  [[nodiscard]] bool GetLongPressFired() const noexcept {
    return long_press_fired_;
  }

 protected:
  void OnStateChanged(WidgetState /*from*/, WidgetState to) override {
    if (on_state_ != nullptr) {
      on_state_(to, GetDragOffset());
    }
  }

 private:
  // Long-press detection coroutine. Suspends for 500ms, then checks if the
  // widget still exists and is still in kDragging state. This is the
  // "state machine as condition lock" pattern: no explicit cancellation is
  // needed; the state check acts as the guard.
  static Task<void> LongPressCoroutine(std::weak_ptr<DragBox> weak_box) {
    co_await Sleep(std::chrono::milliseconds(500));
    auto box = weak_box.lock();
    if (box == nullptr) {
      co_return;  // Widget destroyed.
    }
    if (!box->IsDragging()) {
      co_return;  // Released before timeout; state machine changed.
    }
    box->SetLongPressFired(true);
    box->MarkNeedsBuild();
  }

  StateCallback on_state_{};
  Application* app_ = nullptr;
  bool long_press_fired_ = false;
};

// Root widget for the drag demo.
class DragDemoRoot : public Container {
 public:
  explicit DragDemoRoot(Application* app) : app_(app) {
    SetFlexDirection(FlexDirection::kColumn);
    SetJustifyContent(HAlign::kLeft);
    SetAlignItems(VAlign::kTop);
    SetPadding({.left = 16.0F, .top = 16.0F, .right = 16.0F, .bottom = 16.0F});
    SetBackgroundColor({.r = 242, .g = 242, .b = 247, .a = 255});

    // Title.
    auto title = std::make_shared<Text>("Drag Demo: State + Coroutine");
    title->SetFontSize(18.0F).SetTextColor({.r = 26, .g = 26, .b = 38, .a = 255});
    AddChild(title);

    // Status label.
    status_label_ = std::make_shared<Text>("State: Idle  Offset: (0, 0)");
    status_label_->SetFontSize(14.0F)
        .SetTextColor({.r = 77, .g = 77, .b = 89, .a = 255});
    AddChild(status_label_);

    // Long-press hint.
    hint_label_ = std::make_shared<Text>("Hold 500ms for long-press");
    hint_label_->SetFontSize(12.0F)
        .SetTextColor({.r = 128, .g = 128, .b = 140, .a = 255});
    AddChild(hint_label_);

    // Drag area (a large container for visual reference).
    auto drag_area = std::make_shared<Container>();
    drag_area->SetWidth(360.0F);
    drag_area->SetHeight(280.0F);
    drag_area->SetBackgroundColor({.r = 230, .g = 230, .b = 235, .a = 255});
    drag_area->SetBorderRadius(8.0F);
    drag_area->SetFlexDirection(FlexDirection::kColumn);
    drag_area->SetJustifyContent(HAlign::kCenter);
    drag_area->SetAlignItems(VAlign::kCenter);

    // The draggable box.
    drag_box_ = std::make_shared<DragBox>(
        app_, [this](WidgetState state, const Point& offset) {
          UpdateStatus(state, offset);
        });
    drag_box_->SetWidth(100.0F);
    drag_box_->SetHeight(100.0F);
    drag_box_->SetBackgroundColor({.r = 51, .g = 128, .b = 230, .a = 255});
    drag_box_->SetBorderRadius(12.0F);
    drag_box_->SetFlexDirection(FlexDirection::kColumn);
    drag_box_->SetJustifyContent(HAlign::kCenter);
    drag_box_->SetAlignItems(VAlign::kCenter);

    auto box_label = std::make_shared<Text>("Drag Me");
    box_label->SetFontSize(14.0F)
        .SetTextColor({.r = 255, .g = 255, .b = 255, .a = 255});
    drag_box_->AddChild(box_label);

    drag_area->AddChild(drag_box_);
    AddChild(drag_area);
  }

  [[nodiscard]] std::string_view GetWidgetName() const noexcept override {
    return "DragDemoRoot";
  }

 private:
  void UpdateStatus(WidgetState state, const Point& offset) {
    std::string state_str;
    switch (state) {
      case WidgetState::kIdle:
        state_str = "Idle";
        break;
      case WidgetState::kHovering:
        state_str = "Hovering";
        break;
      case WidgetState::kDragging:
        state_str = "Dragging";
        break;
      default:
        state_str = "Unknown";
        break;
    }
    std::string text = "State: " + state_str + "  Offset: (" +
                       std::to_string(static_cast<int>(offset.x)) + ", " +
                       std::to_string(static_cast<int>(offset.y)) + ")";
    if (drag_box_ != nullptr && drag_box_->GetLongPressFired()) {
      text += "  [Long Press!]";
    }
    status_label_->SetText(text);
    status_label_->MarkNeedsBuild();
  }

  std::shared_ptr<Text> status_label_ = nullptr;
  std::shared_ptr<Text> hint_label_ = nullptr;
  std::shared_ptr<DragBox> drag_box_ = nullptr;
  Application* app_ = nullptr;
};

}  // namespace

std::shared_ptr<Widget> BuildDragPage(BuildContext& context) {
  auto root = std::make_shared<DragDemoRoot>(context.GetApplication());
  return root;
}

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  RouteRegistry::Instance().RegisterRoute("/", BuildDragPage);

  Application app;
  app.SetFontDir("thirdparty/fonts");
  if (!app.Init(argc, argv, 420, 480, "NeoFlux Drag Demo")) {
    LOG(ERROR) << "Failed to initialize application";
    return 1;
  }

  app.PushRoute("/");
  app.Run();
  return 0;
}
