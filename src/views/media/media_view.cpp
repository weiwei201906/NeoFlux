// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux user quick-start - media_view.cpp
//
// Reference media player screen. Shows how to compose a MediaWidget with
// transport controls (play/pause/back). The source is taken from the
// --media_source gflag (default: ./assets/media/sample.mp4).
// =============================================================================

#include "media_view.h"

#include <filesystem>
#include <memory>
#include <string>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "neoflux/widgets/button.h"
#include "neoflux/widgets/container.h"
#include "neoflux/widgets/media_widget.h"
#include "neoflux/widgets/padding.h"
#include "neoflux/widgets/sized_box.h"
#include "neoflux/widgets/text.h"

DEFINE_string(media_source, "./assets/media/sample.mp4",
              "Path or URL passed to the demo MediaWidget on the /media route.");

namespace neoflux_app {

namespace {

constexpr neoflux::Color kBarBackground{.r = 32, .g = 34, .b = 40, .a = 255};
constexpr neoflux::Color kAccent{.r = 80, .g = 140, .b = 255, .a = 255};
constexpr neoflux::Color kTextLight{.r = 235, .g = 237, .b = 242, .a = 255};

}  // namespace

std::shared_ptr<neoflux::Widget> BuildMediaView(
    neoflux::BuildContext& context) {
  auto root = std::make_shared<neoflux::Container>();
  root->SetBackgroundColor({.r = 12, .g = 12, .b = 14, .a = 255})
      .SetFlexDirection(neoflux::FlexDirection::kColumn);

  // Top bar: back button + title.
  auto top_bar = std::make_shared<neoflux::Container>();
  top_bar->SetBackgroundColor(kBarBackground)
      .SetFlexDirection(neoflux::FlexDirection::kRow)
      .SetPadding({.left = 16.0F, .top = 12.0F, .right = 16.0F, .bottom = 12.0F})
      .SetAlignItems(neoflux::VAlign::kCenter);

  auto back_btn = std::make_shared<neoflux::Button>("<- Back");
  back_btn->SetBackgroundColor(kAccent)
      .SetTextColor(kTextLight)
      .SetFontSize(14.0F)
      .SetOnPressed([&context]() { context.PopRoute(); });

  auto title = std::make_shared<neoflux::Text>("Media Player");
  title->SetFontSize(18.0F).SetTextColor(kTextLight);

  top_bar->AddChild(back_btn);
  top_bar->AddChild(std::make_shared<neoflux::SizedBox>(16.0F, 0.0F));
  top_bar->AddChild(title);

  // Video surface: flex-grow to fill remaining vertical space.
  auto media = std::make_shared<neoflux::MediaWidget>();
  // mpv will emit END_FILE/error events if the path does not exist, but warn
  // here up front so the user immediately knows to pass --media_source=<path>
  // instead of seeing a silent black surface. URLs (http://...) are skipped.
  const std::string& source = FLAGS_media_source;
  const bool looks_like_url =
      source.starts_with("http://") || source.starts_with("https://") ||
      source.starts_with("rtsp://") || source.starts_with("rtmp://");
  std::error_code ec;
  if (!looks_like_url && !std::filesystem::exists(source, ec)) {
    LOG(WARNING) << "Media source '" << source
                 << "' does not exist on disk; pass --media_source=<path> to "
                    "play a local file.";
  }
  media->SetSource(source)
      .SetBackgroundColor({.r = 0, .g = 0, .b = 0, .a = 255})
      .Play();

  // Bottom control bar.
  auto bottom_bar = std::make_shared<neoflux::Container>();
  bottom_bar->SetBackgroundColor(kBarBackground)
      .SetFlexDirection(neoflux::FlexDirection::kRow)
      .SetPadding({.left = 16.0F, .top = 12.0F, .right = 16.0F, .bottom = 12.0F})
      .SetAlignItems(neoflux::VAlign::kCenter);

  // Play/pause toggle keeps a shared_ptr so the callback can flip the label.
  auto play_btn = std::make_shared<neoflux::Button>("Pause");
  play_btn->SetBackgroundColor(kAccent).SetTextColor(kTextLight).SetFontSize(14.0F);
  play_btn->SetOnPressed([media, play_btn]() {
    if (media->GetState() == neoflux::MediaState::kPlaying) {
      media->Pause();
      play_btn->SetLabel("Play");
    } else {
      media->Play();
      play_btn->SetLabel("Pause");
    }
  });

  auto source_label = std::make_shared<neoflux::Text>(FLAGS_media_source);
  source_label->SetFontSize(12.0F)
      .SetTextColor({.r = 150, .g = 152, .b = 160, .a = 255});

  bottom_bar->AddChild(play_btn);
  bottom_bar->AddChild(std::make_shared<neoflux::SizedBox>(16.0F, 0.0F));
  bottom_bar->AddChild(source_label);

  root->AddChild(top_bar);

  // Media surface expands to fill the middle.
  auto media_wrap = std::make_shared<neoflux::Container>();
  media_wrap->SetFlexDirection(neoflux::FlexDirection::kColumn)
      .SetFlexGrow(1.0F);
  media_wrap->AddChild(media);
  root->AddChild(media_wrap);

  root->AddChild(bottom_bar);
  return root;
}

}  // namespace neoflux_app
