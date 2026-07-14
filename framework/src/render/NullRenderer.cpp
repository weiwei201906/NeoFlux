#include "neoflux/render/NullRenderer.h"
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "neoflux/core/Widget.h"

namespace neoflux::render {

bool NullRenderer::initialize(int width, int height) {
  width_ = width;
  height_ = height;
  running_ = true;
  return true;
}

void NullRenderer::renderScene(const std::shared_ptr<core::Widget>& root, const core::BuildContext& context) {
  if (!root)
    return;
  frameBuffer_.str("");
  frameBuffer_.clear();
  RenderContext ctx{};
  ctx.width = context.width;
  ctx.height = context.height;
  ctx.native = nullptr;
  ctx.out = &frameBuffer_;
  root->render(ctx);
}

void NullRenderer::pollEvents() { std::this_thread::sleep_for(std::chrono::milliseconds(4)); }

void NullRenderer::requestShutdown() { running_ = false; }

bool NullRenderer::isRunning() const { return running_; }

std::string NullRenderer::lastFramePreview() const { return frameBuffer_.str(); }

}  // namespace neoflux::render