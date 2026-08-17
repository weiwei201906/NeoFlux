// =============================================================================
// NeoFlux - Flex Layout Demo
//
// Demonstrates Taitank flex layout capabilities:
//   - Row and column flex directions
//   - Justify content (left, center, right)
//   - Align items (top, center, bottom)
//   - Padding and margin
//   - Nested containers
//   - Flexible sizing (flex grow)
// =============================================================================

#include <neoflux/app/application.h>
#include <neoflux/widget/button.h>
#include <neoflux/widget/container.h>
#include <neoflux/widget/route_registry.h>
#include <neoflux/widget/text.h>
#include <neoflux/widget/widget.h>

#include <memory>
#include <string>

namespace neoflux {
namespace {

// Creates a labeled colored box for layout demonstration.
std::shared_ptr<Container> MakeBox(const std::string_view label, const Color& bg,
                                   const float width, const float height) {
  auto box = std::make_shared<Container>();
  box->SetBackgroundColor(bg)
      .SetWidth(width)
      .SetHeight(height)
      .SetJustifyContent(HAlign::kCenter)
      .SetAlignItems(VAlign::kCenter);
  auto text = std::make_shared<Text>(std::string(label));
  text->SetFontSize(14.0F).SetTextColor({.r = 255, .g = 255, .b = 255, .a = 255});
  box->AddChild(text);
  return box;
}

// Creates a section label with top spacing via a wrapper container.
std::shared_ptr<Container> MakeSectionLabel(std::string_view text) {
  auto wrapper = std::make_shared<Container>();
  wrapper->SetMargin({.top = 16.0F});
  auto label = std::make_shared<Text>(std::string(text));
  label->SetFontSize(16.0F)
      .SetTextColor({.r = 100, .g = 100, .b = 100, .a = 255});
  wrapper->AddChild(label);
  return wrapper;
}

// Builds a page demonstrating flex layout features.
std::shared_ptr<Widget> BuildFlexPage(BuildContext& /*context*/) {
  auto root = std::make_shared<Container>();
  root->SetFlexDirection(FlexDirection::kColumn)
      .SetBackgroundColor({.r = 245, .g = 245, .b = 245, .a = 255})
      .SetPadding({.left = 16.0F, .top = 16.0F, .right = 16.0F, .bottom = 16.0F});

  // Title.
  auto title = std::make_shared<Text>("Flex Layout Demo");
  title->SetFontSize(24.0F)
      .SetTextColor({.r = 33, .g = 33, .b = 33, .a = 255})
      .SetAlignment(HAlign::kCenter);
  root->AddChild(title);

  // Section 1: Row with center justification.
  root->AddChild(MakeSectionLabel("Row - Center"));
  auto row1 = std::make_shared<Container>();
  row1->SetFlexDirection(FlexDirection::kRow)
      .SetJustifyContent(HAlign::kCenter)
      .SetAlignItems(VAlign::kCenter)
      .SetMargin({.top = 8.0F})
      .SetHeight(60.0F);
  row1->AddChild(MakeBox("A", {.r = 244, .g = 67, .b = 54, .a = 255}, 50, 50));
  row1->AddChild(MakeBox("B", {.r = 76, .g = 175, .b = 80, .a = 255}, 50, 50));
  row1->AddChild(MakeBox("C", {.r = 33, .g = 150, .b = 243, .a = 255}, 50, 50));
  root->AddChild(row1);

  // Section 2: Column with center alignment.
  root->AddChild(MakeSectionLabel("Column - Center"));
  auto col1 = std::make_shared<Container>();
  col1->SetFlexDirection(FlexDirection::kColumn)
      .SetJustifyContent(HAlign::kCenter)
      .SetAlignItems(VAlign::kCenter)
      .SetMargin({.top = 8.0F})
      .SetHeight(140.0F);
  col1->AddChild(MakeBox("1", {.r = 156, .g = 39, .b = 176, .a = 255}, 80, 36));
  col1->AddChild(MakeBox("2", {.r = 255, .g = 152, .b = 0, .a = 255}, 80, 36));
  col1->AddChild(MakeBox("3", {.r = 0, .g = 188, .b = 212, .a = 255}, 80, 36));
  root->AddChild(col1);

  // Section 3: Row with flex grow.
  root->AddChild(MakeSectionLabel("Row - Flex Grow"));
  auto row2 = std::make_shared<Container>();
  row2->SetFlexDirection(FlexDirection::kRow)
      .SetAlignItems(VAlign::kCenter)
      .SetMargin({.top = 8.0F})
      .SetHeight(50.0F);
  auto grow1 = MakeBox("Flex 1", {.r = 121, .g = 85, .b = 72, .a = 255}, 0, 40);
  grow1->SetFlexGrow(1.0F);
  auto grow2 = MakeBox("Flex 2", {.r = 96, .g = 125, .b = 139, .a = 255}, 0, 40);
  grow2->SetFlexGrow(2.0F);
  row2->AddChild(grow1);
  row2->AddChild(grow2);
  root->AddChild(row2);

  // Section 4: Row reverse.
  root->AddChild(MakeSectionLabel("Row Reverse"));
  auto row3 = std::make_shared<Container>();
  row3->SetFlexDirection(FlexDirection::kRowReverse)
      .SetJustifyContent(HAlign::kCenter)
      .SetAlignItems(VAlign::kCenter)
      .SetMargin({.top = 8.0F})
      .SetHeight(50.0F);
  row3->AddChild(MakeBox("1", {.r = 233, .g = 30, .b = 99, .a = 255}, 40, 40));
  row3->AddChild(MakeBox("2", {.r = 63, .g = 81, .b = 181, .a = 255}, 40, 40));
  row3->AddChild(MakeBox("3", {.r = 0, .g = 150, .b = 136, .a = 255}, 40, 40));
  root->AddChild(row3);

  return root;
}

}  // namespace
}  // namespace neoflux

int main(int argc, char** argv) {
  using namespace neoflux;

  RouteRegistry::Instance().RegisterRoute("/", BuildFlexPage);

  Application app;
  if (!app.Init(argc, argv, 400, 520, "NeoFlux Flex Layout Demo")) {
    return 1;
  }
  app.PushRoute("/");
  app.Run();
  return 0;
}
