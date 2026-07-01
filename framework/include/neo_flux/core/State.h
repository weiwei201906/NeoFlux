#pragma once

#include <cstdint>

namespace neoflux {
namespace core {

struct State {
  std::uint64_t tick = 0;
  bool dirty = true;
};

} // namespace core
} // namespace neoflux
