#pragma once

namespace neoflux {
namespace core {
class Widget;
class BuildContext;
}
namespace engine {

class LayoutEngine {
 public:
  void computeLayout(core::Widget& root, const core::BuildContext& context) const;
};

} // namespace engine
} // namespace neoflux
