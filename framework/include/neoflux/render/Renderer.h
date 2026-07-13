#pragma once

#include "neoflux/core/BuildContext.h"
#include "neoflux/core/Widget.h"
#include "neoflux/render/RenderContext.h"

#include <memory>

namespace neoflux {
namespace render {

class Renderer {
 public:
  virtual ~Renderer() = default;
  // Initialize renderer resources (GL context, Skia surfaces, etc.).
  virtual bool initialize(int width, int height) = 0;
  // Perform a render pass for the provided widget tree.
  virtual void renderScene(const std::shared_ptr<core::Widget>& root,
                           const core::BuildContext& context) = 0;
  // Poll window events and maintain the rendering surface.
  virtual void pollEvents() = 0;
  // Request renderer shutdown from other threads.
  virtual void requestShutdown() = 0;
  // Return whether the renderer is still running.
  virtual bool isRunning() const = 0;
};

using RendererPtr = std::unique_ptr<Renderer>;

} // namespace render
} // namespace neoflux
