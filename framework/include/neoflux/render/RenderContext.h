#pragma once

namespace neoflux {
namespace render {

// Minimal, renderer-agnostic context passed to widgets during rendering.
// Implementations will cast/extend this as needed (Skia surface, canvas, etc.).
struct RenderContext {
  void* native = nullptr; // opaque renderer-specific handle (e.g., SkSurface*)
  int width = 0;
  int height = 0;
};

} // namespace render
} // namespace neoflux
