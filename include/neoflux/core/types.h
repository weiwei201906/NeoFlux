// =============================================================================
// NeoFlux - types.h
//
// Common type aliases and forward declarations used across the framework.
// All method implementations are in types.cpp.
// =============================================================================

#ifndef NEOFLUX_CORE_TYPES_H_
#define NEOFLUX_CORE_TYPES_H_

#include <cstddef>
#include <cstdint>

namespace neoflux {

// Signed integer type for sizes and counts (preferred over size_t for
// arithmetic to avoid unsigned underflow bugs).
using isize = std::ptrdiff_t;

// 2D point with floating-point coordinates.
struct Point {
  float x = 0.0F;
  float y = 0.0F;
};

// 2D size with floating-point dimensions.
struct Size {
  float width = 0.0F;
  float height = 0.0F;
};

// Axis-aligned rectangle.
struct Rect {
  float x = 0.0F;
  float y = 0.0F;
  float width = 0.0F;
  float height = 0.0F;

  // Returns the right edge x-coordinate.
  [[nodiscard]] float right() const noexcept;
  // Returns the bottom edge y-coordinate.
  [[nodiscard]] float bottom() const noexcept;
};

// RGBA color with 8-bit per channel.
struct Color {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  uint8_t a = 255;

  // Creates a color from 32-bit ARGB integer.
  [[nodiscard]] static Color FromArgb(uint32_t argb) noexcept;

  // Converts to 32-bit ARGB integer.
  [[nodiscard]] uint32_t ToArgb() const noexcept;
};

// Edge insets for padding/margin.
struct EdgeInsets {
  float left = 0.0F;
  float top = 0.0F;
  float right = 0.0F;
  float bottom = 0.0F;
};

// Layout constraints passed down the widget tree.
struct LayoutConstraints {
  float min_width = 0.0F;
  float max_width = 0.0F;
  float min_height = 0.0F;
  float max_height = 0.0F;
};

// Horizontal alignment enumeration.
enum class HAlign { kLeft, kCenter, kRight };

// Vertical alignment enumeration.
enum class VAlign { kTop, kCenter, kBottom };

// Main axis direction for flex layouts.
enum class Axis { kHorizontal, kVertical };

}  // namespace neoflux

#endif  // NEOFLUX_CORE_TYPES_H_
