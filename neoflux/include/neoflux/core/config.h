// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - core/config.h
//
// Compile-time configuration knobs. These are values that are baked into the
// binary at build time (unlike gflags, which are runtime-tunable) and that
// you are likely to tweak when porting or tuning NeoFlux for a new platform.
//
// Everything here is an `inline constexpr` so it costs no storage and can be
// overridden per TU by -D flags without ODR risk. Keep this file free of
// includes other than <cstddef>/<cstdint>.
// =============================================================================

#ifndef NEOFLUX_CORE_CONFIG_H_
#define NEOFLUX_CORE_CONFIG_H_

#include <cstddef>
#include <cstdint>

namespace neoflux::config {

// Cache line size in bytes. Used to align SPSC queue head/tail so the producer
// and consumer ends sit on separate lines (prevents false sharing).
// x86/ARM server: 64; some Apple M series and newer AMD have 128-byte lines.
#ifndef NEOFLUX_CACHE_LINE_SIZE
inline constexpr std::size_t kCacheLineSize = 64;
#else
inline constexpr std::size_t kCacheLineSize = NEOFLUX_CACHE_LINE_SIZE;
#endif

// Default capacity of the render command SPSC ring queue (number of slots).
// Rounded up to a power of two at runtime; one slot is reserved for full/empty
// distinction, so the usable capacity is (this - 1).
inline constexpr std::uint32_t kDefaultRenderQueueCapacity = 2048;

// Target frame rate when no gflag overrides it.
inline constexpr std::uint16_t kDefaultTargetFps = 60;

// Long-press threshold in milliseconds (Button widget state machine).
inline constexpr std::uint16_t kLongPressThresholdMs = 500;

// Fling scroll velocity threshold (screen heights per second) below which the
// inertia animation stops.
inline constexpr float kFlingStopThreshold = 0.02F;

}  // namespace neoflux::config

#endif  // NEOFLUX_CORE_CONFIG_H_
