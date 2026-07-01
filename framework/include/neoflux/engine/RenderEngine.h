#pragma once

namespace neoflux {
namespace core {
class Widget;
}
namespace engine {

class RenderEngine {
 public:
  void renderWidget(const core::Widget& root) const;
};

} // namespace engine
} // namespace neoflux
