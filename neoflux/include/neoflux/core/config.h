// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - core/config.h
//
// Compile-time configuration knobs. Only values that MUST be constants live
// here (alignas / template arguments / static assertions) -- everything that
// can reasonably be tuned at runtime is a gflags flag defined in core/flags.cpp
// instead (see --help for the runtime surface).
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
// This MUST be a compile-time constant: it feeds alignas() and the per-platform
// cache-topology verification in src/native/.
#ifndef NEOFLUX_CACHE_LINE_SIZE
inline constexpr std::size_t kCacheLineSize = 64;
#else
inline constexpr std::size_t kCacheLineSize = NEOFLUX_CACHE_LINE_SIZE;
#endif

}  // namespace neoflux::config

#endif  // NEOFLUX_CORE_CONFIG_H_
