#include "neoflux/engine/RenderEngine.h"
#include <glog/logging.h>
#include <neoflux/widgets/ButtonWidget.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <memory>
#include <string>
#include <utility>

#include "neoflux/render/NullRenderer.h"
#include "neoflux/render/SkiaGlfwRenderer.h"

namespace neoflux::engine {

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

RenderEngine::RenderEngine(const RenderPipelineConfig& config)
    : pipelineConfig_(config), requestQueue_(1024), running_(false) {}

RenderEngine::~RenderEngine() {
  running_.store(false);
  if (renderThread_.joinable()) {
    renderThread_.join();
  }
}

bool RenderEngine::initialize(int width, int height) {
  if (running_.load()) {
    return true;
  }

  renderContext_.width = width;
  renderContext_.height = height;

  switch (pipelineConfig_.backend) {
    case RenderBackend::kSkia:
      renderer_ = std::make_unique<render::SkiaGlfwRenderer>(pipelineConfig_);
      break;
    case RenderBackend::kNull:
      renderer_ = std::make_unique<render::NullRenderer>();
      break;
    case RenderBackend::kSoftware:
      renderer_ = std::make_unique<render::NullRenderer>();
      break;
  }

  if (!renderer_ || !renderer_->initialize(width, height)) {
    LOG(ERROR) << "RenderEngine: failed to initialize renderer";
    return false;
  }

  running_.store(true);
  renderThread_ = std::thread(&RenderEngine::renderLoop, this);
  return true;
}

void RenderEngine::setPipelineConfig(const RenderPipelineConfig& config) { pipelineConfig_ = config; }

const RenderPipelineConfig& RenderEngine::pipelineConfig() const { return pipelineConfig_; }

const std::string& RenderEngine::lastFramePreview() const { return lastFramePreview_; }

bool RenderEngine::renderWidget(std::shared_ptr<core::Widget> root, const core::BuildContext& context) {
  if (!renderer_) {
    LOG(WARNING) << "RenderEngine: renderer not initialized.";
    return false;
  }

  if (!running_.load()) {
    if (!initialize(context.width, context.height)) {
      return false;
    }
  }

  RenderRequest request;
  request.root = std::move(root);
  request.context = context;
  while (!requestQueue_.try_push(std::move(request))) {
    if (!running_.load()) {
      return false;
    }
    std::this_thread::yield();
  }
  return true;
}

void RenderEngine::renderLoop() {
  while (running_.load() && renderer_ && renderer_->isRunning()) {
    RenderRequest request;
    if (requestQueue_.try_pop(request)) {
      renderer_->renderScene(request.root, request.context);
      lastFramePreview_ = renderer_->lastFramePreview();
    }
    renderer_->pollEvents();
    std::this_thread::sleep_for(std::chrono::milliseconds(0));
  }
}

}  // namespace neoflux::engine