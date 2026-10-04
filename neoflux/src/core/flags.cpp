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

// ---------------------------------------------------------------------------
// Event loop idle pacing
// ---------------------------------------------------------------------------
DEFINE_int32(idle_fps, 15,
             "Idle heart-beat rate for the event loop in frames per second. "
             "When no render request and no coroutine/timer work is pending "
             "for a few consecutive frames, the loop drops to this rate to "
             "save CPU (input events still wake it instantly). 0 disables "
             "idle throttling entirely (always full --target_fps).");

// ---------------------------------------------------------------------------
// Native tuning layer (src/native/)
// ---------------------------------------------------------------------------
// Best-effort OS-level tuning. Everything degrades silently when the OS
// refuses; these flags only change what is *attempted*, never correctness.
DEFINE_bool(native_tuning, true,
            "Master switch for the platform-native tuning layer (thread "
            "scheduling, MMCSS, timer resolution, big-core pinning). "
            "false makes every native entry point a no-op.");

DEFINE_int32(native_render_rt_priority, 1,
             "Linux/Android: SCHED_FIFO real-time priority attempted for the "
             "render thread (1 = lowest RT priority). Used only when "
             "CAP_SYS_NICE is granted; otherwise the nice fallback below is "
             "tried. Range 1..99.");

DEFINE_int32(native_thread_nice, -5,
             "Linux/Android: nice value attempted for the render (fallback) "
             "and UI threads. Negative values require CAP_SYS_NICE and fail "
             "silently without it. Range -20..19.");

DEFINE_int32(native_bigcore_threshold_permille, 950,
             "Big-core detection threshold in permille of the fastest core's "
             "max frequency (950 = 95%). Cores at or above the threshold are "
             "considered 'big' when pinning the render thread. 500..1000; "
             "values below 500 are clamped.");

DEFINE_string(native_mmcss_profile, "Games",
              "Windows: MMCSS (AvSetMmThreadCharacteristicsW) profile name "
              "used when registering the render thread. Ignored on other "
              "platforms and when MMCSS is unavailable.");

DEFINE_int32(native_timer_period_ms, 1,
             "Windows: timer resolution in ms requested via timeBeginPeriod "
             "from the UI thread (0 disables the request). Fixes the ~15.6 ms "
             "condition_variable wait granularity that jitters 60 FPS frame "
             "pacing. Ignored on other platforms.");

// NOTE: there is deliberately no --render_backend flag. The GPU backend is
// tgfx's own compile-time choice (TGFX_USE_* switches, resolved to exactly one
// in thirdparty/CMakeLists.txt) and cannot be changed at runtime.

}  // namespace neoflux
