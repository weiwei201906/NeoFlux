#pragma once

#include "neoflux/render/Renderer.h"

namespace neoflux {
namespace render {

class NullRenderer : public Renderer {
 public:
  explicit NullRenderer() = default;
  ~NullRenderer() override = default;

  bool initialize(int width, int height) override;
  void renderScene(const std::shared_ptr<core::Widget>& root,
                   const core::BuildContext& context) override;
  void pollEvents() override;
  void requestShutdown() override;
  bool isRunning() const override;

 private:
  bool running_ = false;
  int width_ = 0;
  int height_ = 0;
};

} // namespace render
} // namespace neoflux
