// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - core/flags.h
//
// Declarations for the framework's gflags flags. Definitions live in
// flags.cpp; include this header to reference a flag from another
// translation unit.
//
// Keeping the declarations here (rather than re-declaring them ad hoc at
// each use site) means a typo in a flag name fails to compile instead of
// silently linking against a different symbol.
// =============================================================================

#ifndef NEOFLUX_CORE_FLAGS_H_
#define NEOFLUX_CORE_FLAGS_H_

#include <cstdint>
#include <string>

#include <gflags/gflags.h>

namespace neoflux {

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------

// Enable verbose VLOG(1) logging and mirror output to stderr.
DECLARE_bool(verbose_logging);

// ---------------------------------------------------------------------------
// Event loop
// ---------------------------------------------------------------------------

// Target frames per second for the event loop.
DECLARE_int32(target_fps);

// ---------------------------------------------------------------------------
// Render layer
// ---------------------------------------------------------------------------

// Capacity of the render command SPSC ring queue (rounded up to a power of
// two at runtime).
DECLARE_uint64(render_queue_capacity);

// Maximum number of "queue full, dropped commands" warnings per process.
DECLARE_int32(render_queue_drop_log_max);

// Render backend selection. Only "gl" is implemented.
DECLARE_string(render_backend);

}  // namespace neoflux

#endif  // NEOFLUX_CORE_FLAGS_H_
