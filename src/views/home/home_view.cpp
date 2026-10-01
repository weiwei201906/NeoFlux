// =============================================================================
// NeoFlux user quick-start - home_view.cpp
// =============================================================================

#include "home_view.h"

#include "neoflux/widgets/container.h"
#include "neoflux/widgets/text.h"

namespace neoflux_app {

std::shared_ptr<neoflux::Widget> BuildHomeView(
    neoflux::BuildContext& /*context*/) {
  auto root = std::make_shared<neoflux::Container>();
  root->SetBackgroundColor({.r = 245, .g = 246, .b = 250, .a = 255})
      .SetPadding({.left = 40.0F, .top = 48.0F, .right = 40.0F, .bottom = 40.0F});

  const auto title = std::make_shared<neoflux::Text>("NeoFlux Quick Start");
  title->SetFontSize(32.0F).SetTextColor({.r = 20, .g = 20, .b = 24, .a = 255});

  const auto subtitle = std::make_shared<neoflux::Text>(
      "Edit src/views/home/home_view.cpp to build your first screen.");
  subtitle->SetFontSize(16.0F)
      .SetTextColor({.r = 90, .g = 92, .b = 100, .a = 255});

  root->AddChild(title);
  root->AddChild(subtitle);
  return root;
}

}  // namespace neoflux_app
