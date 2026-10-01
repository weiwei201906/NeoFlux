// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - ring_queue.cpp
//
// Explicit instantiation of SpscRingQueue for the framework's render command
// queue. Method definitions live in the header (it is a template); this TU
// forces the RenderCommand specialization's symbols into the neoflux library.
// =============================================================================

#include "neoflux/core/ring_queue.h"
#include "neoflux/renderers/render_command.h"

namespace neoflux {

// Explicit instantiation for the render command queue used by RenderLayer.
// The capacity is configured at runtime via the render_queue_capacity gflag.
template class SpscRingQueue<RenderCommand>;

}  // namespace neoflux
