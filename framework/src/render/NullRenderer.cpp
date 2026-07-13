#include "neoflux/render/NullRenderer.h"
#include "neoflux/util/SPSCQueue.h"
#include "neoflux/core/Widget.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

namespace neoflux {
namespace render {

bool NullRenderer::initialize(int width, int height) {
  width_ = width;
  height_ = height;
  running_ = true;
  return true;
}

void NullRenderer::renderScene(const std::shared_ptr<core::Widget>& root,
                              const core::BuildContext& context) {
  if (!root) return;
  neoflux::render::RenderContext ctx{};
  ctx.width = context.width;
  ctx.height = context.height;
  ctx.native = nullptr;
  root->render(ctx);
}

void NullRenderer::pollEvents() {
  std::this_thread::sleep_for(std::chrono::milliseconds(4));
}

void NullRenderer::requestShutdown() { running_ = false; }

bool NullRenderer::isRunning() const { return running_; }

} // namespace render
} // namespace neoflux
