// =============================================================================
// NeoFlux - ring_queue.cpp
//
// Template implementation include + explicit instantiations of SpscRingQueue
// for types used by the framework.
// =============================================================================

#include "neoflux/render/render_layer.h"

#include "ring_queue_impl.inc"

namespace neoflux {

// Explicit instantiation for the render command queue used by RenderLayer.
template class SpscRingQueue<RenderCommand, kRenderQueueCapacity>;

}  // namespace neoflux
