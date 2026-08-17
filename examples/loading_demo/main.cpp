// =============================================================================
// NeoFlux - Loading Animation Demo
//
// Demonstrates the lightweight widget state machine and C++20 coroutines:
//   - Widget states: kIdle -> kLoading -> kSuccess
//   - OnStateChanged launches a coroutine that drives a progress animation
//   - The coroutine yields each frame and updates progress via MarkNeedsBuild
//   - On completion, transitions to kSuccess
// =============================================================================

#include <neoflux/app/application.h>
#include <neoflux/core/task.h>
#include <neoflux/widget/button.h>
#include <neoflux/widget/container.h>
#include <neoflux/widget/route_registry.h>
#include <neoflux/widget/text.h>
#include <neoflux/widget/widget.h>

#include <memory>
#include <string>

namespace neoflux {
namespace {

// A widget with a state machine that drives a loading animation via coroutine.
//
// States:
//   kIdle     - shows a "Load Data" button
//   kLoading  - shows an animated progress bar (coroutine-driven)
//   kSuccess  - shows "Loaded!" with a success color
//
// Inherits from Container so it can be used as a layout widget directly.
class LoadingWidget : public Container {
 public:
  explicit LoadingWidget(Application* app) : app_(app) {}

  [[nodiscard]] std::string_view GetWidgetName() const noexcept override {
    return "LoadingWidget";
  }

 protected:
  // State transition hook: launch the loading coroutine when entering
  // kLoading. This is the "seam" where the state machine connects to
  // coroutine-driven animation.
  void OnStateChanged(WidgetState from, WidgetState to) override {
    if (to == WidgetState::kLoading && from != WidgetState::kLoading) {
      progress_ = 0.0F;
      if (app_ != nullptr) {
        app_->GetEventLoop().Schedule(LoadingCoroutine());
      }
    }
    MarkNeedsBuild();
  }

  std::shared_ptr<Widget> Build(BuildContext& /*context*/) override {
    auto root = std::make_shared<Container>();
    root->SetFlexDirection(FlexDirection::kColumn)
        .SetJustifyContent(HAlign::kCenter)
        .SetAlignItems(VAlign::kCenter)
        .SetPadding({.left = 24, .top = 24, .right = 24, .bottom = 24});

    switch (GetState()) {
      case WidgetState::kIdle:
        BuildIdleState(*root);
        break;
      case WidgetState::kLoading:
        BuildLoadingState(*root);
        break;
      case WidgetState::kSuccess:
        BuildSuccessState(*root);
        break;
      default:
        break;
    }
    return root;
  }

 private:
  // Coroutine that animates the progress bar from 0 to 100 over ~2 seconds.
  // Each iteration yields one frame, updates progress, and marks the widget
  // dirty so it rebuilds with the new progress value.
  Task<void> LoadingCoroutine() {
    constexpr int kTotalFrames = 120;  // ~2 seconds at 60fps
    for (int frame = 0; frame <= kTotalFrames; ++frame) {
      progress_ = static_cast<float>(frame) / static_cast<float>(kTotalFrames);
      MarkNeedsBuild();
      co_await Yield();
    }
    // Animation complete: transition to success state.
    SetState(WidgetState::kSuccess);
  }

  void BuildIdleState(Container& root) {
    auto title = std::make_shared<Text>("State Machine + Coroutine Demo");
    title->SetFontSize(20.0F)
        .SetTextColor({.r = 33, .g = 33, .b = 33, .a = 255});
    root.AddChild(title);

    auto spacer = std::make_shared<Container>();
    spacer->SetHeight(24.0F);
    root.AddChild(spacer);

    auto btn = std::make_shared<Button>("Start Loading");
    btn->SetFontSize(16.0F)
        .SetBackgroundColor({.r = 33, .g = 150, .b = 243, .a = 255})
        .SetTextColor({.r = 255, .g = 255, .b = 255, .a = 255});
    btn->SetOnPressed([this] { SetState(WidgetState::kLoading); });
    root.AddChild(btn);
  }

  void BuildLoadingState(Container& root) const {
    auto label = std::make_shared<Text>("Loading...");
    label->SetFontSize(18.0F)
        .SetTextColor({.r = 100, .g = 100, .b = 100, .a = 255});
    root.AddChild(label);

    auto spacer = std::make_shared<Container>();
    spacer->SetHeight(16.0F);
    root.AddChild(spacer);

    // Progress bar: track contains fill as child so they overlap correctly.
    auto track = std::make_shared<Container>();
    track->SetWidth(240.0F)
        .SetHeight(12.0F)
        .SetBackgroundColor({.r = 224, .g = 224, .b = 224, .a = 255})
        .SetBorderRadius(6.0F)
        .SetFlexDirection(FlexDirection::kRow)
        .SetAlignItems(VAlign::kTop);
    // Fill sits inside track, left-aligned, width proportional to progress.
    auto fill = std::make_shared<Container>();
    const float fill_width = 240.0F * progress_;
    fill->SetWidth(fill_width)
        .SetHeight(12.0F)
        .SetBackgroundColor({.r = 33, .g = 150, .b = 243, .a = 255})
        .SetBorderRadius(6.0F);
    track->AddChild(fill);
    root.AddChild(track);

    auto spacer2 = std::make_shared<Container>();
    spacer2->SetHeight(16.0F);
    root.AddChild(spacer2);

    const int percent = static_cast<int>(progress_ * 100.0F);
    auto pct = std::make_shared<Text>(std::to_string(percent) + "%");
    pct->SetFontSize(14.0F)
        .SetTextColor({.r = 100, .g = 100, .b = 100, .a = 255});
    root.AddChild(pct);
  }

  void BuildSuccessState(Container& root) {
    auto icon = std::make_shared<Text>("OK");
    icon->SetFontSize(32.0F)
        .SetTextColor({.r = 76, .g = 175, .b = 80, .a = 255});
    root.AddChild(icon);

    auto label = std::make_shared<Text>("Load Complete!");
    label->SetFontSize(20.0F)
        .SetTextColor({.r = 76, .g = 175, .b = 80, .a = 255});
    root.AddChild(label);

    auto spacer = std::make_shared<Container>();
    spacer->SetHeight(24.0F);
    root.AddChild(spacer);

    auto btn = std::make_shared<Button>("Reset");
    btn->SetFontSize(16.0F)
        .SetBackgroundColor({.r = 76, .g = 175, .b = 80, .a = 255})
        .SetTextColor({.r = 255, .g = 255, .b = 255, .a = 255});
    btn->SetOnPressed([this] { SetState(WidgetState::kIdle); });
    root.AddChild(btn);
  }

  Application* app_ = nullptr;
  float progress_ = 0.0F;
};

std::shared_ptr<Widget> BuildLoadingPage(BuildContext& context) {
  auto root = std::make_shared<Container>();
  root->SetFlexDirection(FlexDirection::kColumn)
      .SetBackgroundColor({.r = 250, .g = 250, .b = 250, .a = 255})
      .SetJustifyContent(HAlign::kCenter)
      .SetAlignItems(VAlign::kCenter);

  auto loader = std::make_shared<LoadingWidget>(context.GetApplication());
  root->AddChild(loader);
  return root;
}

}  // namespace
}  // namespace neoflux

int main(int argc, char** argv) {
  using namespace neoflux;

  RouteRegistry::Instance().RegisterRoute("/", BuildLoadingPage);

  Application app;
  if (!app.Init(argc, argv, 360, 480, "NeoFlux Loading Demo")) {
    return 1;
  }
  app.PushRoute("/");
  app.Run();
  return 0;
}
