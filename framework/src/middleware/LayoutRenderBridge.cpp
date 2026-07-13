#include "neoflux/core/Bridge.h"
#include "neoflux/core/Widget.h"
#include "neoflux/engine/LayoutEngine.h"
#include "neoflux/engine/RenderEngine.h"
#include <glog/logging.h>

namespace neoflux {

Bridge::Bridge() = default;

void Bridge::initialize(std::shared_ptr<core::Widget> root,
                        engine::LayoutEngine* layoutEngine,
                        engine::RenderEngine* renderEngine) {
  root_ = std::move(root);
  layoutEngine_ = layoutEngine;
  renderEngine_ = renderEngine;
}

void Bridge::update(core::State& state, const core::BuildContext& context) {
  if (!root_ || !layoutEngine_) {
    LOG(WARNING) << "Bridge: unable to update layout because root or engine is missing.";
    return;
  }

  state.tick += 1;
  state.dirty = true;
  lastContext_ = context;
  layoutEngine_->computeLayout(*root_, context);
}

void Bridge::render(const core::State& state) const {
  if (!root_ || !renderEngine_) {
    LOG(WARNING) << "Bridge: unable to render because root or engine is missing.";
    return;
  }

  if (!state.dirty) {
    LOG(INFO) << "Bridge: skipping render because state is not dirty.";
    return;
  }

  renderEngine_->renderWidget(root_, lastContext_);
}

core::Widget* Bridge::rootWidget() const {
  return root_.get();
}

} // namespace neoflux
