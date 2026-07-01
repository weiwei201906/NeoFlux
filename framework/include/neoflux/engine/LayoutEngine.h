#pragma once

#include "../core/BuildContext.h"

namespace neoflux {
namespace core {
class Widget;
}
namespace engine {

class LayoutEngine {
 public:
  void computeLayout(core::Widget& root, const core::BuildContext& context) const;
};

} // namespace engine
} // namespace neoflux
