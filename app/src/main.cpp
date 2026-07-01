// GNU General Public License v3.0
// Author: weiwei201906
// Date: 2026-07-01

#include <glog/logging.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>

#include <neoflux/core/Bridge.h>
#include <neoflux/core/BuildContext.h>
#include <neoflux/core/State.h>
#include <neoflux/core/Widget.h>
#include <neoflux/engine/LayoutEngine.h>
#include <neoflux/engine/RenderEngine.h>
#include <neoflux/widgets/ButtonWidget.h>
#include <neoflux/widgets/ContainerWidget.h>
#include <neoflux/widgets/TextWidget.h>

namespace {

using neoflux::core::BuildContext;
using neoflux::core::State;
using neoflux::core::Widget;
using neoflux::engine::RenderBackend;
using neoflux::engine::RenderEngine;
using neoflux::engine::RenderPipelineConfig;
using neoflux::widgets::ButtonWidget;
using neoflux::widgets::ContainerWidget;
using neoflux::widgets::TextWidget;

RenderPipelineConfig parsePipelineConfig(int argc, char** argv) {
  RenderPipelineConfig config;
  config.platform = neoflux::engine::platformName();

  for (int index = 1; index < argc; ++index) {
    const std::string_view arg{argv[index]};
    if (arg == "--backend" && index + 1 < argc) {
      const std::string_view value{argv[++index]};
      if (value == "skia") {
        config.backend = RenderBackend::kSkia;
      } else if (value == "software") {
        config.backend = RenderBackend::kSoftware;
      } else if (value == "null") {
        config.backend = RenderBackend::kNull;
      }
    } else if (arg == "--skia-api" && index + 1 < argc) {
      const std::string_view value{argv[++index]};
      if (value == "vulkan") {
        config.skiaApi = neoflux::engine::SkiaGraphicsApi::kVulkan;
      } else if (value == "software") {
        config.skiaApi = neoflux::engine::SkiaGraphicsApi::kSoftware;
      } else {
        config.skiaApi = neoflux::engine::SkiaGraphicsApi::kOpenGL;
      }
    } else if (arg == "--platform" && index + 1 < argc) {
      config.platform = argv[++index];
    } else if (arg == "--disable-aa") {
      config.antialiasing = false;
    } else if (arg == "--disable-cache") {
      config.enableCaching = false;
    } else if (arg == "--gpu") {
      config.useGpu = true;
    }
  }

  return config;
}

std::shared_ptr<Widget> buildDemoWidget() {
  auto root = std::make_shared<ContainerWidget>(ContainerWidget::Direction::Column);
  auto title = std::make_shared<TextWidget>("Hello, NeoFlux!");
  auto subtitle = std::make_shared<TextWidget>("Skia pipeline demo");
  auto button = std::make_shared<ButtonWidget>("Render now");

  button->setOnClick([] { std::cout << "Button clicked" << '\n'; });

  root->addChild(title);
  root->addChild(subtitle);
  root->addChild(button);
  return root;
}

}  // namespace

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);

  const auto config = parsePipelineConfig(argc, argv);
  auto root = buildDemoWidget();

  BuildContext context;
  context.width = 800;
  context.height = 600;
  context.debug = true;

  State state;
  neoflux::engine::LayoutEngine layoutEngine;
  RenderEngine renderEngine{config};
  neoflux::Bridge bridge;

  bridge.initialize(root, &layoutEngine, &renderEngine);
  bridge.update(state, context);
  bridge.render(state);

  LOG(INFO) << "App configured render backend='" << neoflux::engine::renderBackendName(config.backend)
            << "' on platform='" << config.platform << "'";
  std::cout << "NeoFlux sample: backend=" << neoflux::engine::renderBackendName(config.backend)
            << " skia-api=" << neoflux::engine::skiaApiName(config.skiaApi)
            << " platform=" << config.platform << " antialiasing="
            << (config.antialiasing ? "on" : "off") << '\n';
  std::cout << "[UI Scene] Header: Hello, NeoFlux!\n";
  std::cout << "[UI Scene] Body: A composable widget tree with text, buttons, and layout.\n";
  std::cout << "[UI Scene] Footer: Render pipeline ready for Skia + "
            << neoflux::engine::skiaApiName(config.skiaApi) << "\n";
  return EXIT_SUCCESS;
}
