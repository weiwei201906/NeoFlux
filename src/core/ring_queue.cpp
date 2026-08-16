// =============================================================================
// NeoFlux - ring_queue.cpp
//
// Template implementation include + explicit instantiations of SpscRingQueue
// for types used by the framework.
// =============================================================================

#include "neoflux/core/ring_queue.h"
#include "neoflux/render/render_command.h"

#include "ring_queue_impl.inc"

namespace neoflux {

// Explicit instantiation for the render command queue used by RenderLayer.
// The capacity is configured at runtime via the render_queue_capacity gflag.
template class SpscRingQueue<RenderCommand>;

}  // namespace neoflux
