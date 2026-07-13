#include "gtest/gtest.h"
#include <filesystem>
#include <neoflux/core/BuildContext.h>
#include <neoflux/core/Widget.h>
#include <neoflux/engine/RenderEngine.h>
#include <neoflux/platform/PlatformWindow.h>
#include <neoflux/widgets/ButtonWidget.h>
#include <neoflux/widgets/ContainerWidget.h>
#include <neoflux/widgets/TextWidget.h>
#include "MyAppWidget.h"

TEST(NeoFluxPlaceholder, WidgetTreeIsCreated) {
  auto root = createAppWidget();
  ASSERT_NE(root, nullptr);
  EXPECT_FALSE(root->children().empty());
}

TEST(NeoFluxPlaceholder, LayoutComputesBounds) {
  neoflux::core::BuildContext context;
  context.width = 640;
  context.height = 480;

  auto root = createAppWidget();
  root->setBounds(0, 0, context.width, context.height);
  root->layout(context);

  EXPECT_EQ(root->width(), context.width);
  EXPECT_EQ(root->height(), context.height);
}

TEST(NeoFluxPlaceholder, TerminalPreviewUsesFlexLayout) {
  neoflux::engine::RenderPipelineConfig config;
  config.backend = neoflux::engine::RenderBackend::kSkia;
  config.skiaApi = neoflux::engine::SkiaGraphicsApi::kOpenGL;
  config.platform = "linux";

  neoflux::engine::RenderEngine engine{config};
  const auto preview = engine.buildTerminalPreview("Hello, NeoFlux!", "Skia pipeline demo", "Render now");

  EXPECT_NE(preview.find("Hello, NeoFlux!"), std::string::npos);
  EXPECT_NE(preview.find("Skia pipeline demo"), std::string::npos);
  EXPECT_NE(preview.find("Render now"), std::string::npos);
  EXPECT_NE(preview.find("+"), std::string::npos);
}

TEST(NeoFluxPlaceholder, EmplaceChildKeepsOwnershipAndBuilding) {
  auto root = std::make_shared<neoflux::widgets::ContainerWidget>(
      neoflux::widgets::ContainerWidget::Direction::Column);
  auto title = root->emplaceChild<neoflux::widgets::TextWidget>("Hello");
  auto button = root->emplaceChild<neoflux::widgets::ButtonWidget>("Click");

  ASSERT_EQ(root->children().size(), 2U);
  EXPECT_NE(title, nullptr);
  EXPECT_NE(button, nullptr);
  EXPECT_EQ(title->name(), "Hello");
}

TEST(NeoFluxPlaceholder, RenderEngineProducesRasterArtifact) {
  neoflux::engine::RenderPipelineConfig config;
  config.backend = neoflux::engine::RenderBackend::kSkia;
  config.platform = "linux";

  neoflux::engine::RenderEngine engine{config};
  auto root = std::make_shared<neoflux::widgets::ContainerWidget>(
      neoflux::widgets::ContainerWidget::Direction::Column);
  root->emplaceChild<neoflux::widgets::TextWidget>("Rendered UI");
  root->emplaceChild<neoflux::widgets::ButtonWidget>("Save");

  const auto outputPath = std::filesystem::temp_directory_path() / "neoflux_render_test.ppm";
  EXPECT_TRUE(engine.renderToFile(*root, outputPath.string(), 320, 180));
  EXPECT_TRUE(std::filesystem::exists(outputPath));
  std::filesystem::remove(outputPath);
}

TEST(NeoFluxPlaceholder, PlatformWindowCanOpenAndClose) {
  neoflux::platform::PlatformWindow window{"NeoFlux Test Window"};

  window.open();
  EXPECT_TRUE(window.isOpen());

  window.close();
  EXPECT_FALSE(window.isOpen());
}
