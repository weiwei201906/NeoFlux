// GNU General Public License v3.0
// Author: weiwei201906
// Date: 2026-07-01

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>
#include <thread>

#include <neoflux/core/Bridge.h>
#include <neoflux/core/BuildContext.h>
#include <neoflux/core/State.h>
#include <neoflux/core/Widget.h>
#include <neoflux/engine/LayoutEngine.h>
#include <neoflux/engine/RenderEngine.h>
#include <neoflux/platform/PlatformWindow.h>
#include "MyAppWidget.h"


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

DEFINE_string(backend, "skia", "Rendering backend: skia, software, null");
DEFINE_string(skia_api, "vulkan", "Skia graphics API: opengl, vulkan, software");
DEFINE_string(platform, std::string{neoflux::engine::platformName()}, "Platform label");
DEFINE_bool(disable_aa, false, "Disable antialiasing");
DEFINE_bool(disable_cache, false, "Disable render cache");
DEFINE_bool(gpu, false, "Prefer a GPU path");

RenderPipelineConfig parsePipelineConfig() {
  RenderPipelineConfig config;
  config.platform = FLAGS_platform;
  config.antialiasing = !FLAGS_disable_aa;
  config.enableCaching = !FLAGS_disable_cache;
  config.useGpu = FLAGS_gpu;

  if (FLAGS_backend == "skia") {
    config.backend = RenderBackend::kSkia;
  } else if (FLAGS_backend == "software") {
    config.backend = RenderBackend::kSoftware;
  } else if (FLAGS_backend == "null") {
    config.backend = RenderBackend::kNull;
  }

  if (FLAGS_skia_api == "vulkan") {
    config.skiaApi = neoflux::engine::SkiaGraphicsApi::kVulkan;
  } else if (FLAGS_skia_api == "software") {
    config.skiaApi = neoflux::engine::SkiaGraphicsApi::kSoftware;
  } else {
    config.skiaApi = neoflux::engine::SkiaGraphicsApi::kOpenGL;
  }

  return config;
}

std::shared_ptr<Widget> buildDemoWidget() {
  return createAppWidget();
}

}  // namespace

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  const auto config = parsePipelineConfig();
  auto root = buildDemoWidget();

  BuildContext context;
  context.width = 800;
  context.height = 600;
  context.debug = true;

  State state;
  neoflux::engine::LayoutEngine layoutEngine;
  RenderEngine renderEngine{config};
  neoflux::Bridge bridge;

  neoflux::platform::PlatformWindow window{"NeoFlux Demo"};
  window.open();

  if (!renderEngine.initialize(context.width, context.height)) {
    LOG(WARNING) << "Failed to start render thread";
  }

  bridge.initialize(root, &layoutEngine, &renderEngine);

  for (int frame = 0; window.isOpen(); ++frame) {
    root->setBounds(0, 0, context.width, context.height);
    layoutEngine.computeLayout(*root, context);
    bridge.update(state, context);
    bridge.render(state);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  LOG(INFO) << "App configured render backend='" << neoflux::engine::renderBackendName(config.backend)
            << "' on platform='" << config.platform << "'";
  std::cout << "NeoFlux sample: backend=" << neoflux::engine::renderBackendName(config.backend)
            << " skia-api=" << neoflux::engine::skiaApiName(config.skiaApi)
            << " platform=" << config.platform << " antialiasing="
            << (config.antialiasing ? "on" : "off") << '\n';

  window.close();
  return EXIT_SUCCESS;
}
