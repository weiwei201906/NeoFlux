// =============================================================================
// NeoFlux - Font Demo
//
// Demonstrates the font system:
//   - Default font (no SetFont call)
//   - Explicit font selection via SetFont(name)
//   - Multiple font sizes and colors
//   - CJK text rendering
//
// Place your .ttf/.otf/.ttc files in thirdparty/fonts/ and reference them
// by filename stem (without extension).
// =============================================================================

#include <neoflux/app/application.h>
#include <neoflux/widget/container.h>
#include <neoflux/widget/route_registry.h>
#include <neoflux/widget/text.h>
#include <neoflux/widget/widget.h>

#include <memory>

namespace neoflux {
namespace {

// Builds a page demonstrating various font configurations.
std::shared_ptr<Widget> BuildFontPage(BuildContext& /*context*/) {
  auto root = std::make_shared<Container>();
  root->SetFlexDirection(FlexDirection::kColumn)
      .SetJustifyContent(HAlign::kCenter)
      .SetAlignItems(VAlign::kCenter)
      .SetPadding({.left = 32.0F, .top = 32.0F, .right = 32.0F, .bottom = 32.0F})
      .SetBackgroundColor({.r = 250, .g = 250, .b = 250, .a = 255});

  // Title: default font, large size.
  auto title = std::make_shared<Text>("NeoFlux Font Demo");
  title->SetFontSize(28.0F)
      .SetTextColor({.r = 33, .g = 33, .b = 33, .a = 255})
      .SetAlignment(HAlign::kCenter);
  root->AddChild(title);

  // Explicit font selection by name (filename stem without extension).
  auto explicit_font =
      std::make_shared<Text>("Explicit: SetFont(NotoSansSC-Regular)");
  explicit_font->SetFontSize(16.0F)
      .SetTextColor({.r = 0, .g = 102, .b = 204, .a = 255})
      .SetFont("NotoSansSC-Regular");
  root->AddChild(explicit_font);

  // Default font (no SetFont call - uses first discovered font).
  auto default_font = std::make_shared<Text>("Default: no SetFont() call");
  default_font->SetFontSize(16.0F)
      .SetTextColor({.r = 102, .g = 102, .b = 102, .a = 255});
  root->AddChild(default_font);

  // CJK text rendering.
  auto cjk = std::make_shared<Text>(
      "CJK Support: hello world / ni hao shi jie");
  cjk->SetFontSize(18.0F)
      .SetTextColor({.r = 33, .g = 33, .b = 33, .a = 255});
  root->AddChild(cjk);

  // Small size.
  auto small = std::make_shared<Text>("Small: 12px");
  small->SetFontSize(12.0F)
      .SetTextColor({.r = 153, .g = 153, .b = 153, .a = 255});
  root->AddChild(small);

  // Large size.
  auto large = std::make_shared<Text>("Large: 36px");
  large->SetFontSize(36.0F)
      .SetTextColor({.r = 204, .g = 51, .b = 51, .a = 255});
  root->AddChild(large);

  // Note.
  auto note = std::make_shared<Text>(
      "Add fonts to thirdparty/fonts/ and call SetFont(name)");
  note->SetFontSize(13.0F)
      .SetTextColor({.r = 120, .g = 120, .b = 120, .a = 255});
  root->AddChild(note);

  return root;
}

}  // namespace
}  // namespace neoflux

int main(int argc, char** argv) {
  using namespace neoflux;

  RouteRegistry::Instance().RegisterRoute("/", BuildFontPage);

  Application app;
  app.SetFontDir("./fonts/");
  if (!app.Init(argc, argv, 500, 400, "NeoFlux Font Demo")) {
    return 1;
  }
  app.PushRoute("/");
  app.Run();
  return 0;
}
