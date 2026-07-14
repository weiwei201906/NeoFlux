#pragma once

#include "neoflux/render/Renderer.h"
#include "neoflux/engine/RenderEngine.h"

#include <atomic>
#include <memory>
#include <sstream>

namespace neoflux::render {

// Placeholder Skia+GLFW renderer. Currently provides a minimal implementation
// that can be replaced with a real Skia+GLFW backend later. Keeps API stable.
class SkiaGlfwRenderer : public Renderer {
 public:
  explicit SkiaGlfwRenderer(const neoflux::engine::RenderPipelineConfig& config);
  ~SkiaGlfwRenderer() override;

  bool initialize(int width, int height) override;
  void renderScene(const std::shared_ptr<core::Widget>& root, const core::BuildContext& context) override;
  void pollEvents() override;
  void requestShutdown() override;
  bool isRunning() const override;
  std::string lastFramePreview() const override;

 private:
  neoflux::engine::RenderPipelineConfig config_;
  std::atomic<bool> running_{false};
  std::ostringstream frameBuffer_;
  std::size_t frameIndex_ = 0;
};

}  // namespace neoflux::render