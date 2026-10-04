// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - core/flags.cpp
//
// Single definition point for every gflags flag the framework exposes.
//
// Flags used to be declared from whichever .cpp first needed them, which
// scattered the CLI surface across the tree and made it easy to document a
// flag that no longer existed (or to add one nobody documented). Defining
// them all here keeps the runtime CLI in one reviewable place; other
// translation units pull in just the DECLARE_* they use via
// neoflux/core/flags.h.
//
// Adding a flag: define it here, declare it in flags.h, and document it in
// both READMEs.
// =============================================================================

#include "neoflux/core/flags.h"

#include <gflags/gflags.h>

#include "neoflux/core/config.h"

namespace neoflux {

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------
DEFINE_bool(verbose_logging, false,
            "Enable verbose VLOG(1) logging and mirror output to stderr.");

// ---------------------------------------------------------------------------
// Event loop
// ---------------------------------------------------------------------------
DEFINE_int32(target_fps, neoflux::config::kDefaultTargetFps,
             "Target frames per second for the event loop.");

// ---------------------------------------------------------------------------
// Render layer
// ---------------------------------------------------------------------------
DEFINE_uint64(render_queue_capacity,
              neoflux::config::kDefaultRenderQueueCapacity,
              "Capacity of the render command SPSC ring queue. "
              "One slot is reserved for full/empty distinction, so the "
              "maximum storable commands are (capacity - 1).");

DEFINE_int32(render_queue_drop_log_max, 10,
             "Maximum number of times a 'render command queue full, dropped "
             "commands' warning is emitted per process. After this many "
             "occurrences, subsequent drops are counted silently.");

// NOTE: there is deliberately no --render_backend flag. tgfx permits exactly
// one GPU backend per build, so the backend is selected at CMake configure time
// via -DNEOFLUX_BACKEND=<gl|vulkan|d3d12|metal> (see thirdparty/CMakeLists.txt)
// and cannot be changed at runtime.

}  // namespace neoflux
