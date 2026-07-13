#include "neoflux/render/SkiaGlfwRenderer.h"

#include <glog/logging.h>
#include <utility>

namespace neoflux {
namespace render {

SkiaGlfwRenderer::SkiaGlfwRenderer(const neoflux::engine::RenderPipelineConfig& config)
    : config_(config) {}

SkiaGlfwRenderer::~SkiaGlfwRenderer() = default;

bool SkiaGlfwRenderer::initialize(int width, int height) {
  LOG(INFO) << "SkiaGlfwRenderer: placeholder initialize " << width << "x" << height;
  running_.store(true);
  return true;
}

void SkiaGlfwRenderer::renderScene(const std::shared_ptr<core::Widget>& root,
                                   const core::BuildContext& context) {
  if (!root) return;
  // For now call legacy widget render with RenderContext stub.
  neoflux::render::RenderContext ctx{};
  ctx.width = context.width;
  ctx.height = context.height;
  root->render(ctx);
}

void SkiaGlfwRenderer::pollEvents() {
  // No-op placeholder. Real implementation polls GLFW events here.
}

void SkiaGlfwRenderer::requestShutdown() {
  running_.store(false);
}

bool SkiaGlfwRenderer::isRunning() const { return running_.load(); }

} // namespace render
} // namespace neoflux
