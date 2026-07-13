#pragma once

#include "neoflux/render/Renderer.h"
#include "neoflux/engine/RenderEngine.h"

#include <atomic>
#include <memory>

namespace neoflux {
namespace render {

// Placeholder Skia+GLFW renderer. Currently provides a minimal implementation
// that can be replaced with a real Skia+GLFW backend later. Keeps API stable.
class SkiaGlfwRenderer : public Renderer {
 public:
  explicit SkiaGlfwRenderer(const neoflux::engine::RenderPipelineConfig& config);
  ~SkiaGlfwRenderer() override;

  bool initialize(int width, int height) override;
  void renderScene(const std::shared_ptr<core::Widget>& root,
                   const core::BuildContext& context) override;
  void pollEvents() override;
  void requestShutdown() override;
  bool isRunning() const override;

 private:
  neoflux::engine::RenderPipelineConfig config_;
  std::atomic<bool> running_{false};
};

} // namespace render
} // namespace neoflux
