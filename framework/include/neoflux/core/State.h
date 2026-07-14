#pragma once

#include <cstdint>

namespace neoflux::core {

struct State {
  std::size_t tick = 0;
  bool dirty = true;
};

}  // namespace neoflux::core