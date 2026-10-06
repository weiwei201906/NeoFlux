// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - cache_topology_apple.cpp
//
// macOS / iOS cache-topology probing + prefetch hints, via sysctl.
//
// Placement: drops into neoflux/src/native/apple/ (or src/native/, per the
// coordinator's CMake glob). Exactly one cache_topology_*.cpp is compiled per
// platform; this one for __APPLE__.
//
// Detection strategy:
//   * hw.cachelinesize   -> coherence line size. Apple M-series (arm64)
//                           returns 128; Intel Macs return 64. This is the
//                           single most important value: it drives whether
//                           the compile-time 64B alignment is sufficient.
//   * hw.l1dcachesize    -> per-core L1 data cache size.
//   * hw.l2cachesize     -> L2 size.
//   * hw.l3cachesize     -> L3 size; this key does NOT exist on every Apple
//                           part (notably many Apple Silicon SoCs expose no
//                           L3 sysctl). A failed sysctlbyname() leaves the
//                           field at 0 = "unknown", which is the contract.
//
// Prefetch uses __builtin_prefetch (an intrinsic, NOT inline asm). On arm64
// Clang lowers it to PRFM; on x86 to PREFETCHT0. Correct host placement: the
// prefetch does not synchronise or fault, so a hint on a cold address is safe.
// =============================================================================

#include "native/native_tuning.h"

#include <sys/sysctl.h>

#include <cstddef>

#include "neoflux/core/config.h"
#include "native/asm/asm_symbols.h"

#include <glog/logging.h>

namespace neoflux::native {
namespace {

/// Reads a `size_t`-valued sysctl into `out`. Returns false when the key is
/// absent (e.g. hw.l3cachesize on Apple Silicon) or the size changed under us.
bool ReadSysctlSize(const char* name, std::size_t* out) {
  std::size_t value = 0;
  std::size_t len = sizeof(value);
  if (sysctlbyname(name, &value, &len, nullptr, 0) != 0) {
    return false;
  }
  if (len != sizeof(value)) {
    return false;  // Unexpected width: leave the field untouched (unknown).
  }
  *out = value;
  return true;
}

}  // namespace

CacheInfo DetectCacheTopologySysctlImpl() noexcept {
  CacheInfo info;  // line_size = 64 default -> safe fallback.

  // Coherence line size: 128 on Apple M-series, 64 on Intel Macs. This is
  // the value VerifyCacheLineConfig() compares against config::kCacheLineSize.
  std::size_t line = 0;
  if (ReadSysctlSize("hw.cachelinesize", &line) && line != 0) {
    info.line_size = line;
  }

  // Capacities: absent keys (common for L3 on Apple Silicon) stay 0 = unknown.
  ReadSysctlSize("hw.l1dcachesize", &info.l1d_bytes);
  ReadSysctlSize("hw.l2cachesize", &info.l2_bytes);
  ReadSysctlSize("hw.l3cachesize", &info.l3_bytes);

  return info;
}

void PrefetchForRead(const void* p) noexcept {
#if defined(NEOFLUX_NATIVE_ASM_PREFETCH)
  // PRFM PSTL1KEEP on arm64, PREFETCHT0 on x86-64. Issued by hand-written
  // assembly rather than __builtin_prefetch so every platform reaches the same
  // instruction through the same C ABI.
  neoflux_prefetch_read(p);
#else
  (void)p;
#endif
}

void PrefetchForWrite(const void* p) noexcept {
#if defined(NEOFLUX_NATIVE_ASM_PREFETCH)
  // PRFM PSTL1STRM on arm64 (streaming, keeps L1 for useful lines); PREFETCHT0
  // on x86-64, where the write-allocate hint is an AMD extension that is not
  // part of the baseline.
  neoflux_prefetch_write(p);
#else
  (void)p;
#endif
}

void VerifyCacheLineConfig() noexcept {
  const CacheInfo info = DetectCacheTopology();
  LOG_FIRST_N(INFO, 1) << "native: cache topology detected -- line="
                       << info.line_size << "B L1d=" << info.l1d_bytes
                       << "B L2=" << info.l2_bytes << "B L3=" << info.l3_bytes
                       << "B";

  if (info.line_size > config::kCacheLineSize) {
    // Canonical case on Apple M-series: 128B hardware line vs 64B default
    // alignment. Warn once and point at the build-time fix.
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
  static const CacheInfo kCached = DetectCacheTopologySysctlImpl();
  return kCached;
}

}  // namespace neoflux::native
