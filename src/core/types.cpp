// =============================================================================
// NeoFlux - types.cpp
//
// Implementation of inline methods moved from types.h.
// =============================================================================

#include "neoflux/core/types.h"

#include <cstdint>

namespace neoflux {

float Rect::right() const noexcept { return x + width; }

float Rect::bottom() const noexcept { return y + height; }

Color Color::FromArgb(const uint32_t argb) noexcept {
  return Color{
      .r = static_cast<uint8_t>((argb >> 16U) & 0xFFU),
      .g = static_cast<uint8_t>((argb >> 8U) & 0xFFU),
      .b = static_cast<uint8_t>(argb & 0xFFU),
      .a = static_cast<uint8_t>((argb >> 24U) & 0xFFU),
  };
}

uint32_t Color::ToArgb() const noexcept {
  return (static_cast<uint32_t>(a) << 24U) |
         (static_cast<uint32_t>(r) << 16U) |
         (static_cast<uint32_t>(g) << 8U) | static_cast<uint32_t>(b);
}

}  // namespace neoflux
