#pragma once

#include "BuildContext.h"
#include "State.h"

#include <memory>

namespace neoflux {
namespace core {
class Widget;
}
namespace engine {
class LayoutEngine;
class RenderEngine;
}

class Bridge {
 public:
  Bridge();
  void initialize(std::shared_ptr<core::Widget> root,
                  engine::LayoutEngine* layoutEngine,
                  engine::RenderEngine* renderEngine);
  void update(core::State& state, const core::BuildContext& context);
  void render(const core::State& state) const;
  [[nodiscard]] core::Widget* rootWidget() const;

 private:
  std::shared_ptr<core::Widget> root_;
  engine::LayoutEngine* layoutEngine_ = nullptr;
  engine::RenderEngine* renderEngine_ = nullptr;
  core::BuildContext lastContext_;
};
} // namespace neoflux
