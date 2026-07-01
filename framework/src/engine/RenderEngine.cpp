#include "neoflux/engine/RenderEngine.h"
#include "neoflux/core/Widget.h"
#include <glog/logging.h>

namespace neoflux {
namespace engine {

void RenderEngine::renderWidget(const core::Widget& root) const {
  LOG(INFO) << "RenderEngine: rendering widget tree from root='" << root.name() << "'";
  root.render();
}

} // namespace engine
} // namespace neoflux
