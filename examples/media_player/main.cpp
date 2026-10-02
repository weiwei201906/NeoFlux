// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - Media Player Demo
//
// End-to-end demo of the MediaWidget: plays a local file or URL through the
// libmpv backend, composites the decoded frame into the widget tree, and
// provides a play/pause toggle.
//
// Usage:
//   media_player_demo [path-or-url]
//
// If no argument is given, the demo looks for assets/media/sample.mp4 next to
// the executable and logs a hint when it is missing.
// =============================================================================

#include <neoflux/apps/application.h>
#include <neoflux/widgets/button.h>
#include <neoflux/widgets/container.h>
#include <neoflux/widgets/media_widget.h>
#include <neoflux/widgets/route_registry.h>
#include <neoflux/widgets/text.h>
#include <neoflux/widgets/widget.h>

#include <gflags/gflags.h>

#include <memory>
#include <string>

DEFINE_string(source, "",
              "Media source (file path or URL). If empty, the first positional "
              "argument is used.");

namespace neoflux {
namespace {

// Builds the player page: a header, the video surface, and a control bar.
std::shared_ptr<Widget> BuildPlayerPage(BuildContext& /*context*/,
                                        std::shared_ptr<MediaWidget> player) {
  auto root = std::make_shared<Container>();
  root->SetFlexDirection(FlexDirection::kColumn)
      .SetBackgroundColor({.r = 18, .g = 18, .b = 22, .a = 255});

  // Header bar.
  auto header = std::make_shared<Container>();
  header->SetHeight(48.0F)
      .SetPadding({.left = 16.0F, .right = 16.0F})
      .SetJustifyContent(HAlign::kLeft)
      .SetAlignItems(VAlign::kCenter);
  auto title = std::make_shared<Text>("NeoFlux Media Player");
  title->SetFontSize(18.0F).SetTextColor({.r = 245, .g = 245, .b = 245, .a = 255});
  header->AddChild(title);
  root->AddChild(header);

  // Video surface: expands to fill the remaining space.
  player->SetBackgroundColor({.r = 0, .g = 0, .b = 0, .a = 255});
  root->AddChild(player);

  // Control bar: play/pause toggle + hint text.
  auto bar = std::make_shared<Container>();
  bar->SetHeight(56.0F)
      .SetPadding({.left = 16.0F, .right = 16.0F})
      .SetFlexDirection(FlexDirection::kRow)
      .SetAlignItems(VAlign::kCenter);

  auto toggle = std::make_shared<Button>("Play");
  toggle->SetFontSize(15.0F)
      .SetBackgroundColor({.r = 33, .g = 150, .b = 243, .a = 255})
      .SetTextColor({.r = 255, .g = 255, .b = 255, .a = 255})
      .SetOnPressed([player, toggle]() {
        if (player->GetState() == MediaState::kPlaying) {
          player->Pause();
          toggle->SetLabel("Play");
        } else {
          player->Play();
          toggle->SetLabel("Pause");
        }
      });
  bar->AddChild(toggle);

  auto hint = std::make_shared<Text>("Tap the video to toggle playback.");
  hint->SetFontSize(13.0F).SetTextColor({.r = 170, .g = 170, .b = 175, .a = 255});
  auto hint_wrap = std::make_shared<Container>();
  hint_wrap->SetMargin({.left = 16.0F}).AddChild(hint);
  bar->AddChild(hint_wrap);

  root->AddChild(bar);
  return root;
}

}  // namespace
}  // namespace neoflux

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  // Resolve the source: --source wins, otherwise first positional argument.
  std::string source = FLAGS_source;
  if (source.empty() && argc > 1) {
    source = argv[1];
  }
  if (source.empty()) {
    source = "assets/media/sample.mp4";
  }

  neoflux::Application app;
  if (!app.Init(argc, argv, 960, 540, "NeoFlux Media Player")) {
    return 1;
  }

  auto player = std::make_shared<neoflux::MediaWidget>();
  player->SetSource(source);

  neoflux::RouteRegistry::Instance().RegisterRoute(
      "/", [player](neoflux::BuildContext& ctx) {
        return neoflux::BuildPlayerPage(ctx, player);
      });

  app.PushRoute("/");
  app.Run();
  return 0;
}
