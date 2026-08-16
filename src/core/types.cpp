// =============================================================================
// NeoFlux - types.cpp
//
// Implementation of inline methods moved from types.h.
// =============================================================================

#include "neoflux/core/types.h"

namespace neoflux {

float Rect::right() const noexcept { return x + width; }

float Rect::bottom() const noexcept { return y + height; }

Color Color::FromArgb(uint32_t argb) noexcept {
  return Color{
      static_cast<uint8_t>((argb >> 16) & 0xFF),
      static_cast<uint8_t>((argb >> 8) & 0xFF),
      static_cast<uint8_t>(argb & 0xFF),
      static_cast<uint8_t>((argb >> 24) & 0xFF),
  };
}

uint32_t Color::ToArgb() const noexcept {
  return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(r) << 16) |
         (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
}

}  // namespace neoflux
