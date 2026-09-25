// =============================================================================
// NeoFlux - views/about_view.cpp
//
// About page implementation. Renders the framework version and a short
// architecture description. Stateless: content never changes.
// =============================================================================

#include "views/about_view.h"

#include <neoflux/widget/container.h>
#include <neoflux/widget/sized_box.h>
#include <neoflux/widget/text.h>

#include <memory>

namespace neoflux {

AboutView::AboutView(BuildContext& /*context*/) {}

std::shared_ptr<Widget> AboutView::Build(BuildContext& /*context*/) {
  auto root = std::make_shared<Container>();
  root->SetBackgroundColor({.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF})
      .SetPadding({.left = 24.0F, .top = 24.0F, .right = 24.0F,
                   .bottom = 24.0F});

  auto title = std::make_shared<Text>("About NeoFlux");
  title->SetFontSize(26.0F)
      .SetTextColor({.r = 0x1A, .g = 0x1A, .b = 0x1A, .a = 0xFF});

  auto body = std::make_shared<Text>(
      "Two-layer C++20 UI framework: the Application layer runs business "
      "logic and Taitank flex layout; the Render layer consumes an SPSC "
      "ring queue and draws with tgfx (mobile) or OpenGL (desktop).");
  body->SetFontSize(14.0F)
      .SetTextColor({.r = 0x55, .g = 0x55, .b = 0x55, .a = 0xFF});

  root->AddChild(title);
  root->AddChild(std::make_shared<SizedBox>(0.0F, 16.0F));
  root->AddChild(body);
  return root;
}

}  // namespace neoflux
