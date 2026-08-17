// =============================================================================
// NeoFlux - Scroll View Demo
//
// Demonstrates the ScrollView widget:
//   - Vertical scrolling with mouse wheel
//   - Content larger than the viewport
//   - Clip rect and translate offset for scrolling
//   - Nested containers and text items
// =============================================================================

#include <neoflux/app/application.h>
#include <neoflux/widget/button.h>
#include <neoflux/widget/container.h>
#include <neoflux/widget/route_registry.h>
#include <neoflux/widget/scroll_view.h>
#include <neoflux/widget/text.h>
#include <neoflux/widget/widget.h>

#include <memory>
#include <string>

namespace neoflux {
namespace {

// Creates a list item with a colored background and label.
std::shared_ptr<Container> MakeListItem(int index, const Color& bg) {
  auto item = std::make_shared<Container>();
  item->SetBackgroundColor(bg)
      .SetHeight(60.0F)
      .SetMargin({.bottom = 8.0F})
      .SetPadding({.left = 16.0F, .top = 0.0F, .right = 16.0F, .bottom = 0.0F})
      .SetJustifyContent(HAlign::kCenter)
      .SetAlignItems(VAlign::kCenter);
  auto label = std::make_shared<Text>("Item " + std::to_string(index));
  label->SetFontSize(18.0F).SetTextColor({.r = 255, .g = 255, .b = 255, .a = 255});
  item->AddChild(label);
  return item;
}

// Builds a scrollable list page.
std::shared_ptr<Widget> BuildScrollPage(BuildContext& /*context*/) {
  auto root = std::make_shared<Container>();
  root->SetFlexDirection(FlexDirection::kColumn)
      .SetBackgroundColor({.r = 245, .g = 245, .b = 245, .a = 255});

  // Header (fixed, not scrolled).
  auto header = std::make_shared<Container>();
  header->SetHeight(56.0F)
      .SetBackgroundColor({.r = 33, .g = 150, .b = 243, .a = 255})
      .SetJustifyContent(HAlign::kCenter)
      .SetAlignItems(VAlign::kCenter);
  auto title = std::make_shared<Text>("Scroll View Demo");
  title->SetFontSize(20.0F).SetTextColor({.r = 255, .g = 255, .b = 255, .a = 255});
  header->AddChild(title);
  root->AddChild(header);

  // Scrollable content area.
  auto scroll = std::make_shared<ScrollView>();

  auto content = std::make_shared<Container>();
  content->SetFlexDirection(FlexDirection::kColumn)
      .SetPadding({.left = 16.0F, .top = 16.0F, .right = 16.0F, .bottom = 16.0F});

  // Add many list items to make content exceed viewport height.
  const Color colors[] = {
      {.r = 244, .g = 67, .b = 54, .a = 255},  {.r = 233, .g = 30, .b = 99, .a = 255},
      {.r = 156, .g = 39, .b = 176, .a = 255}, {.r = 103, .g = 58, .b = 183, .a = 255},
      {.r = 63, .g = 81, .b = 181, .a = 255},  {.r = 33, .g = 150, .b = 243, .a = 255},
      {.r = 0, .g = 188, .b = 212, .a = 255},   {.r = 0, .g = 150, .b = 136, .a = 255},
      {.r = 76, .g = 175, .b = 80, .a = 255},   {.r = 139, .g = 195, .b = 74, .a = 255},
      {.r = 205, .g = 220, .b = 57, .a = 255},  {.r = 255, .g = 193, .b = 7, .a = 255},
      {.r = 255, .g = 152, .b = 0, .a = 255},   {.r = 255, .g = 87, .b = 34, .a = 255},
  };
  for (int i = 0; i < 14; ++i) {
    content->AddChild(MakeListItem(i + 1, colors[i % 14]));
  }

  scroll->SetContent(content);
  root->AddChild(scroll);
  return root;
}

}  // namespace
}  // namespace neoflux

int main(int argc, char** argv) {
  using namespace neoflux;

  RouteRegistry::Instance().RegisterRoute("/", BuildScrollPage);

  Application app;
  app.SetFontDir("thirdparty/fonts");
  if (!app.Init(argc, argv, 360, 480, "NeoFlux Scroll Demo")) {
    return 1;
  }
  app.PushRoute("/");
  app.Run();
  return 0;
}
