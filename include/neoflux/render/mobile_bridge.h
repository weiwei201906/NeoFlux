// =============================================================================
// NeoFlux - mobile_bridge.h
//
// Factory function for the mobile platform bridge. The concrete MobileBridge
// class is defined in mobile_bridge.cpp and compiled only on mobile targets
// (Android / iOS). This header provides the cross-platform factory declaration.
// =============================================================================

#ifndef NEOFLUX_RENDER_MOBILE_BRIDGE_H_
#define NEOFLUX_RENDER_MOBILE_BRIDGE_H_

#include <memory>

#include "neoflux/render/platform_bridge.h"

namespace neoflux {

// Creates a mobile platform bridge from a native surface handle.
//   Android: ANativeWindow* from NativeActivity / SurfaceView.
//   iOS:     UIView* or CAMetalLayer* from the view hierarchy.
// Returns nullptr on desktop platforms (stub).
[[nodiscard]] std::unique_ptr<PlatformBridge> CreateMobileBridge(
    void* native_surface, int width, int height);

}  // namespace neoflux

#endif  // NEOFLUX_RENDER_MOBILE_BRIDGE_H_
