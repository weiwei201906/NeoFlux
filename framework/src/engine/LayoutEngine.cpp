#include "neoflux/engine/LayoutEngine.h"
#include "neoflux/core/Widget.h"

namespace neoflux {
namespace engine {

void LayoutEngine::computeLayout(core::Widget& root, const core::BuildContext& context) const {
  root.setBounds(0, 0, context.width, context.height);
  root.layout(context);
}

} // namespace engine
} // namespace neoflux
