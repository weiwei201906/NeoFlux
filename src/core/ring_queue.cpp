// =============================================================================
// NeoFlux - ring_queue.cpp
//
// Explicit instantiations of SpscRingQueue for types used by the framework.
// Template method implementations are in ring_queue_internal.h.
// =============================================================================

#include "neoflux/core/ring_queue_internal.h"
#include "neoflux/render/render_command.h"

namespace neoflux {

// Explicit instantiation for the render command queue used by RenderLayer.
template class SpscRingQueue<RenderCommand, kRenderQueueCapacity>;

}  // namespace neoflux
