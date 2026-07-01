#include "neoflux/engine/RenderEngine.h"

#include "neoflux/core/Widget.h"

#include <glog/logging.h>
#include <utility>

namespace neoflux {
namespace engine {

namespace {
constexpr std::string_view kDefaultPlatform = "linux";
}  // namespace

std::string_view platformName() {
#if defined(_WIN32)
  return "windows";
#elif defined(__APPLE__)
  return "macos";
#else
  return kDefaultPlatform;
#endif
}

std::string_view renderBackendName(RenderBackend backend) {
  switch (backend) {
    case RenderBackend::kSkia:
      return "skia";
    case RenderBackend::kSoftware:
      return "software";
    case RenderBackend::kNull:
      return "null";
  }
  return "unknown";
}

std::string_view skiaApiName(SkiaGraphicsApi api) {
  switch (api) {
    case SkiaGraphicsApi::kOpenGL:
      return "opengl";
    case SkiaGraphicsApi::kVulkan:
      return "vulkan";
    case SkiaGraphicsApi::kSoftware:
      return "software";
  }
  return "unknown";
}

RenderEngine::RenderEngine(RenderPipelineConfig config) : pipelineConfig_(std::move(config)) {}

void RenderEngine::setPipelineConfig(RenderPipelineConfig config) {
  pipelineConfig_ = std::move(config);
}

const RenderPipelineConfig& RenderEngine::pipelineConfig() const {
  return pipelineConfig_;
}

void RenderEngine::renderWidget(const core::Widget& root) const {
  LOG(INFO) << "RenderEngine: rendering widget tree from root='" << root.name() << "' via "
            << renderBackendName(pipelineConfig_.backend) << " on " << pipelineConfig_.platform;
  if (pipelineConfig_.backend == RenderBackend::kSkia) {
    LOG(INFO) << "RenderEngine: Skia pipeline using " << skiaApiName(pipelineConfig_.skiaApi);
  }
  if (pipelineConfig_.antialiasing) {
    LOG(INFO) << "RenderEngine: antialiasing enabled";
  }
  if (pipelineConfig_.enableCaching) {
    LOG(INFO) << "RenderEngine: caching enabled";
  }
  root.render();
}

} // namespace engine
} // namespace neoflux
