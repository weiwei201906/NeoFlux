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

// ---------------------------------------------------------------------------
// Event loop idle pacing
// ---------------------------------------------------------------------------

// Idle heart-beat rate (fps) after a few frames without work; 0 disables
// idle throttling. See flags.cpp for the full description.
DECLARE_int32(idle_fps);

// ---------------------------------------------------------------------------
// Native tuning layer (src/native/)
// ---------------------------------------------------------------------------

// Master switch: false turns every native tuning entry point into a no-op.
DECLARE_bool(native_tuning);

// Linux/Android: SCHED_FIFO priority attempted for the render thread.
DECLARE_int32(native_render_rt_priority);

// Linux/Android: nice value attempted for render (fallback) and UI threads.
DECLARE_int32(native_thread_nice);

// Big-core frequency threshold in permille of the fastest core (500..1000).
DECLARE_int32(native_bigcore_threshold_permille);

// Windows: MMCSS profile name for the render thread registration.
DECLARE_string(native_mmcss_profile);

// Windows: timeBeginPeriod resolution in ms (0 = do not request).
DECLARE_int32(native_timer_period_ms);

// No backend flag: the GPU backend is tgfx's own compile-time choice
// (TGFX_USE_* switches, resolved in thirdparty/CMakeLists.txt).

}  // namespace neoflux

#endif  // NEOFLUX_CORE_FLAGS_H_
