#include "neoflux/render/SkiaGlfwRenderer.h"

#include <glog/logging.h>
#include <iostream>
#include <utility>

namespace neoflux::render {

SkiaGlfwRenderer::SkiaGlfwRenderer(const engine::RenderPipelineConfig& config) : config_(config) {}

SkiaGlfwRenderer::~SkiaGlfwRenderer() = default;

bool SkiaGlfwRenderer::initialize(int width, int height) {
  LOG(INFO) << "SkiaGlfwRenderer: initialize " << width << "x" << height;
  running_.store(true);
  frameBuffer_.str("");
  frameBuffer_.clear();
  return true;
}

void SkiaGlfwRenderer::renderScene(const std::shared_ptr<core::Widget>& root, const core::BuildContext& context) {
  if (!root)
    return;

  frameBuffer_.str("");
  frameBuffer_.clear();
  frameBuffer_ << "[render-thread] frame " << ++frameIndex_ << "\n";
  frameBuffer_ << "[render-thread] size " << context.width << "x" << context.height << "\n";

  RenderContext ctx{};
  ctx.width = context.width;
  ctx.height = context.height;
  ctx.out = &frameBuffer_;
  root->render(ctx);
}

void SkiaGlfwRenderer::pollEvents() {
  if (!frameBuffer_.str().empty()) {
    std::cout << "\033[2J\033[H" << frameBuffer_.str() << std::flush;
  }
}

void SkiaGlfwRenderer::requestShutdown() { running_.store(false); }

bool SkiaGlfwRenderer::isRunning() const { return running_.load(); }

std::string SkiaGlfwRenderer::lastFramePreview() const { return frameBuffer_.str(); }

}  // namespace neoflux::render