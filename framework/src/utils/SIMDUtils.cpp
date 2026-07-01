#include <array>

namespace neoflux {
namespace utils {

std::array<float, 4> add4(const std::array<float, 4>& a, const std::array<float, 4>& b) {
  return {a[0] + b[0], a[1] + b[1], a[2] + b[2], a[3] + b[3]};
}

std::array<float, 4> multiply4(const std::array<float, 4>& a, float scalar) {
  return {a[0] * scalar, a[1] * scalar, a[2] * scalar, a[3] * scalar};
}

} // namespace utils
} // namespace neoflux
