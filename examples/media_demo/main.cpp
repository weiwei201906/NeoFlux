// =============================================================================
// NeoFlux - Media player example
//
// Demonstrates the integrated MediaWidget backed by libmpv:
//   - Video texture composited directly into the widget tree
//   - Play/pause via tap or button
//   - Seek and volume controls
//   - Taitank flex layout
//
// Requirements: libmpv must be installed (https://mpv.io/).
//   Windows: download libmpv dev package, set mpv_DIR in CMake
//   Linux:   sudo apt install libmpv-dev
//   macOS:   brew install mpv
// =============================================================================

#include <neoflux/app/application.h>
#include <neoflux/widget/button.h>
#include <neoflux/widget/container.h>
#include <neoflux/widget/media_widget.h>
#include <neoflux/widget/route_registry.h>
#include <neoflux/widget/text.h>
#include <neoflux/widget/text_field.h>
#include <neoflux/widget/widget.h>

#include <memory>
#include <string>

namespace neoflux {
namespace {

class MediaPlayerWidget : public StatefulWidget {
 public:
  [[nodiscard]] std::string_view GetWidgetName() const noexcept override {
    return "MediaPlayerWidget";
  }

  [[nodiscard]] std::unique_ptr<State<StatefulWidget>> CreateState() override;
};

class MediaPlayerState : public State<StatefulWidget> {
 public:
  [[nodiscard]] std::shared_ptr<Widget> Build(BuildContext& /*context*/) override {
    auto col = std::make_shared<Container>();
    col->SetFlexDirection(FlexDirection::kColumn)
        .SetJustifyContent(HAlign::kCenter)
        .SetAlignItems(VAlign::kCenter)
        .SetPadding({.left = 20.0F, .top = 20.0F, .right = 20.0F, .bottom = 20.0F})
        .SetBackgroundColor({.r = 30, .g = 30, .b = 40, .a = 255});

    auto title = std::make_shared<Text>("NeoFlux Media Player");
    title->SetFontSize(22.0F)
        .SetTextColor({.r = 255, .g = 255, .b = 255, .a = 255});

    media_widget_ = std::make_shared<MediaWidget>();
    media_widget_->SetBackgroundColor({.r = 15, .g = 15, .b = 20, .a = 255});

    auto url_field = std::make_shared<TextField>();
    url_field->SetPlaceholder("Enter media file path or URL...");
    url_field->SetFontSize(14.0F);
    url_field->SetBackgroundColor({.r = 50, .g = 50, .b = 60, .a = 255});
    url_field->SetTextColor({.r = 255, .g = 255, .b = 255, .a = 255});
    url_field->SetPlaceholderColor({.r = 150, .g = 150, .b = 160, .a = 255});
    url_field->SetOnSubmit([this](std::string_view text) {
      media_path_ = std::string(text);
      if (media_widget_ != nullptr) {
        media_widget_->SetSource(media_path_);
        media_widget_->Play();
      }
    });

    auto btn_row = std::make_shared<Container>();
    btn_row->SetFlexDirection(FlexDirection::kRow)
        .SetJustifyContent(HAlign::kCenter)
        .SetAlignItems(VAlign::kCenter);

    auto play_btn = std::make_shared<Button>("Play");
    play_btn->SetFontSize(14.0F)
        .SetBackgroundColor({.r = 66, .g = 133, .b = 244, .a = 255})
        .SetTextColor({.r = 255, .g = 255, .b = 255, .a = 255})
        .SetOnPressed([this] {
          if (media_widget_ != nullptr && !media_path_.empty()) {
            media_widget_->SetSource(media_path_);
            media_widget_->Play();
          }
        });

    auto pause_btn = std::make_shared<Button>("Pause");
    pause_btn->SetFontSize(14.0F)
        .SetBackgroundColor({.r = 200, .g = 160, .b = 60, .a = 255})
        .SetTextColor({.r = 255, .g = 255, .b = 255, .a = 255})
        .SetOnPressed([this] {
          if (media_widget_ != nullptr) {
            media_widget_->Pause();
          }
        });

    auto stop_btn = std::make_shared<Button>("Stop");
    stop_btn->SetFontSize(14.0F)
        .SetBackgroundColor({.r = 200, .g = 60, .b = 60, .a = 255})
        .SetTextColor({.r = 255, .g = 255, .b = 255, .a = 255})
        .SetOnPressed([this] {
          if (media_widget_ != nullptr) {
            media_widget_->Stop();
          }
        });

    btn_row->AddChild(play_btn);
    btn_row->AddChild(pause_btn);
    btn_row->AddChild(stop_btn);

    auto hint = std::make_shared<Text>(
        "Enter a file path or URL, then press Play. libmpv required.");
    hint->SetFontSize(12.0F)
        .SetTextColor({.r = 160, .g = 160, .b = 170, .a = 255})
        .SetAlignment(HAlign::kCenter);

    auto wrap = [](std::shared_ptr<Widget> child, float margin_bottom) {
      auto c = std::make_shared<Container>();
      c->SetMargin({.bottom = margin_bottom});
      c->SetChild(child);
      return c;
    };

    col->AddChild(wrap(title, 16.0F));
    col->AddChild(wrap(media_widget_, 16.0F));
    col->AddChild(wrap(url_field, 12.0F));
    col->AddChild(wrap(btn_row, 12.0F));
    col->AddChild(hint);
    return col;
  }

 private:
  std::string media_path_{};
  std::shared_ptr<MediaWidget> media_widget_{};
};

std::unique_ptr<State<StatefulWidget>> MediaPlayerWidget::CreateState() {
  return std::make_unique<MediaPlayerState>();
}

std::shared_ptr<Widget> BuildMediaPage(BuildContext& /*context*/) {
  auto root = std::make_shared<Container>();
  root->SetFlexDirection(FlexDirection::kColumn)
      .SetJustifyContent(HAlign::kCenter)
      .SetAlignItems(VAlign::kCenter)
      .SetBackgroundColor({.r = 30, .g = 30, .b = 40, .a = 255});
  root->AddChild(std::make_shared<MediaPlayerWidget>());
  return root;
}

}  // namespace
}  // namespace neoflux

int main(int argc, char** argv) {
  using namespace neoflux;

  RouteRegistry::Instance().RegisterRoute("/", BuildMediaPage);

  Application app;
  app.SetFontDir("./fonts/");
  if (!app.Init(argc, argv, 540, 520, "NeoFlux Media Demo")) {
    return 1;
  }
  app.PushRoute("/");
  app.Run();
  return 0;
}
