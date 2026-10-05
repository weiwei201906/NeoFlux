// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - cache_topology_win32.cpp
//
// Windows cache-topology probing + prefetch hints, via CPUID.
//
// Placement: drops into neoflux/src/native/windows/ (or src/native/, per the
// coordinator's CMake glob). Exactly one cache_topology_*.cpp is compiled per
// platform; this one for _WIN32.
//
// Detection strategy (all intrinsic -- the project forbids inline asm):
//   * Leaf 1, EBX[15:8] = CLFLUSH line size in 8-byte units
//       -> coherence line size = field * 8. This is the false-sharing line.
//   * Leaf 4 ("deterministic cache parameters"): for subleaf 0,1,2,... until
//     EAX[4:0] (cache type) == 0:
//         type             = EAX[4:0]       (1=data, 2=instr, 3=unified)
//         level            = EAX[7:5]
//         line_size_bytes  = (EBX[11:0]  + 1)   [threads sharing]
//         partitions       = (EBX[21:12] + 1)   [physical line partitions]
//         ways             = (EBX[31:22] + 1)   [associativity]
//         sets             =  ECX         + 1
//         total_size       = ways * partitions * line_size * sets
//   * Only data-bearing caches (type 1 Data or 3 Unified) feed l1d/l2/l3.
//   * Loop termination: subleaf 0 is "no more caches" (type == 0), or leaf 4
//     itself is unsupported (max_leaf < 4). We also cap the subleaf walk.
//
// __cpuidex(int regs[4], int leaf, int subleaf) is an intrinsic provided by
// BOTH MSVC (<intrin.h>, all modern toolchains) and MinGW-w64 (GCC/Clang's
// cpuid.h path is not used: __cpuidex is declared by <intrin.h> there too).
// It is the subleaf-capable sibling of __cpuid(), needed here because leaf 4
// enumerates one cache per subleaf. No inline asm.
//
// Prefetch uses _mm_prefetch (SSE intrinsic from <xmmintrin.h>, pulled in by
// <intrin.h>) -- also intrinsic, no asm.
// =============================================================================

#include "native/native_tuning.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <intrin.h>

#include <cstddef>
#include <cstdint>

#include "neoflux/core/config.h"

#include <glog/logging.h>

namespace neoflux::native {
namespace {

/// CPUID leaf-4 cache-type encodings (Intel SDM Vol.2, CPUID leaf 04H).
///
/// The field these come from is EAX[4:0], so an underlying type of one byte is
/// all the value can ever need; `int` only made the constants 4x wider than the
/// register field they name.
enum : std::uint8_t {
  kCacheTypeNull = 0,           // "no more caches" sentinel / leaves the loop.
  kCacheTypeData = 1,           // data cache
  kCacheTypeInstruction = 2,    // instruction cache
  kCacheTypeUnified = 3,        // unified (data + instruction)
};

/// True for the cache types that carry data and therefore count toward the
/// L1d/L2/L3 capacities.
bool IsDataBearing(int type) {
  return type == kCacheTypeData || type == kCacheTypeUnified;
}

}  // namespace

CacheInfo DetectCacheTopologyCpuidImpl() noexcept {
  CacheInfo info;  // line_size = 64 default -> safe fallback.
  int regs[4] = {0, 0, 0, 0};

  // Highest supported basic leaf.
  __cpuid(regs, 0);
  const int max_leaf = regs[0];
  if (max_leaf < 1) {
    return info;  // Should be impossible on any x86 that boots Windows.
  }

  // Leaf 1: EBX[15:8] = CLFLUSH line size, in 8-byte units.
  __cpuid(regs, 1);
  const int clflush_line_units = (regs[1] >> 8) & 0xFF;
  if (clflush_line_units != 0) {
    info.line_size = static_cast<std::size_t>(clflush_line_units) * 8;
  }

  if (max_leaf < 4) {
    // Pre-Nehalem CPU: no deterministic-cache-parameter leaf. We still have
    // the CLFLUSH line size (or the 64B default); capacities stay 0.
    return info;
  }

  // Leaf 4 walk: one cache per subleaf, terminated by type == 0. Cap the walk
  // defensively (a well-formed CPU has < 16 cache levels/subleaves).
  for (int subleaf = 0; subleaf < 16; ++subleaf) {
    __cpuidex(regs, 4, subleaf);
    const int type = regs[0] & 0x1F;
    if (type == kCacheTypeNull) {
      break;  // No further caches reported.
    }
    const int level = (regs[0] >> 5) & 0x7;

    const std::size_t line_size =
        static_cast<std::size_t>((regs[1] & 0x0FFF) + 1);        // EBX[11:0]
    const std::size_t partitions =
        static_cast<std::size_t>(((regs[1] >> 12) & 0x03FF) + 1);  // EBX[21:12]
    const std::size_t ways =
        static_cast<std::size_t>(((regs[1] >> 22) & 0x03FF) + 1);  // EBX[31:22]
    const std::size_t sets = static_cast<std::size_t>(regs[2]) + 1;  // ECX
    const std::size_t total =
        ways * partitions * line_size * sets;

    // Prefer the L1D's own line size for line_size: it is the coherence line
    // of the cache that actually holds the SPSC queue head/tail. The CLFLUSH
    // value is a reliable cross-check/fallback when a cache reports nonsense.
    if (level == 1 && type == kCacheTypeData && line_size != 0) {
      info.line_size = line_size;
    }

    if (!IsDataBearing(type)) {
      continue;  // Instruction caches do not contribute capacity.
    }
    if (level == 1) {
      info.l1d_bytes = total > info.l1d_bytes ? total : info.l1d_bytes;
    } else if (level == 2) {
      info.l2_bytes = total > info.l2_bytes ? total : info.l2_bytes;
    } else if (level == 3) {
      info.l3_bytes = total > info.l3_bytes ? total : info.l3_bytes;
    }
  }

  return info;
}

void PrefetchForRead(const void* p) noexcept {
  // _MM_HINT_T0: bring into all cache levels (temporal locality). The default
  // form of _mm_prefetch is a read prefetch.
  _mm_prefetch(static_cast<const char*>(p), _MM_HINT_T0);
}

void PrefetchForWrite(const void* p) noexcept {
  // SSE has no dedicated write hint; _MM_HINT_T0 is the portable choice and
  // is what MSVC/Clang lower to a plain PREFETCHT0. Kept distinct from the
  // read form for interface symmetry and to allow a PREFETCHW path later.
  _mm_prefetch(static_cast<const char*>(p), _MM_HINT_T0);
}

void VerifyCacheLineConfig() noexcept {
  const CacheInfo info = DetectCacheTopology();
  LOG_FIRST_N(INFO, 1) << "native: cache topology detected -- line="
                       << info.line_size << "B L1d=" << info.l1d_bytes
                       << "B L2=" << info.l2_bytes << "B L3=" << info.l3_bytes
                       << "B";

  if (info.line_size > config::kCacheLineSize) {
    LOG_FIRST_N(WARNING, 1)
        << "native: runtime cache line (" << info.line_size
        << "B) exceeds compile-time config::kCacheLineSize ("
        << config::kCacheLineSize
        << "B); SPSC queue head/tail may still share a coherence line -> "
           "false sharing. Rebuild with -DNEOFLUX_CACHE_LINE_SIZE="
        << info.line_size << " to fix it.";
  } else if (info.line_size < config::kCacheLineSize) {
    LOG_FIRST_N(INFO, 1)
        << "native: compile-time cache line (" << config::kCacheLineSize
        << "B) is more conservative than runtime (" << info.line_size
        << "B); alignment is safe.";
  } else {
    LOG_FIRST_N(INFO, 1) << "native: compile-time cache line matches runtime ("
                        << info.line_size << "B)";
  }
}

CacheInfo DetectCacheTopology() noexcept {
  // Cache topology is a process-lifetime invariant: probe once, then serve
  // the cached snapshot. Magic static => thread-safe one-shot evaluation.
  static const CacheInfo kCached = DetectCacheTopologyCpuidImpl();
  return kCached;
}

}  // namespace neoflux::native
