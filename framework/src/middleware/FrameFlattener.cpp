#include "neoflux/core/Widget.h"
#include <vector>

namespace neoflux::middleware {

std::vector<std::shared_ptr<core::Widget>> flatten(const std::shared_ptr<core::Widget>& root) {
  std::vector<std::shared_ptr<core::Widget>> result;
  if (!root) {
    return result;
  }

  result.push_back(root);
  for (auto const& child : root->children()) {
    auto nested = flatten(child);
    result.insert(result.end(), nested.begin(), nested.end());
  }
  return result;
}

}  // namespace neoflux::middleware