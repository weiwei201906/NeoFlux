// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - cache_topology_linux.cpp
//
// Linux (and Android) cache-topology probing + prefetch hints.
//
// Placement: drops into neoflux/src/native/linux/ (or src/native/, depending
// on the CMake glob the coordinator wires up). Exactly one cache_topology_*.cpp
// is compiled per platform; this one for __linux__ (covers Android too).
//
// Detection strategy:
//   * Read the CPU cache hierarchy from sysfs:
//       /sys/devices/system/cpu/cpu0/cache/index{0..N}/{level,type,
//                                                 coherency_line_size,size}
//     The `type` field is "Data"/"Instruction"/"Unified"; `size` looks like
//     "48K" / "1280K" / "16M" and needs K/M/G suffix parsing.
//   * line_size  <- coherency_line_size of the first level-1 Data cache.
//     (coherency_line_size, not the possibly-larger "cache line" for
//     non-coherent caches, is the one that matters for false sharing.)
//   * l1d/l2/l3  <- size of the largest Data/Unified cache at that level.
//   * If sysfs is absent (container/VM without a mounted cpu tree) we fall
//     back to line_size = 64 and keep every capacity 0 = "unknown".
//
// Prefetch uses __builtin_prefetch (an intrinsic, NOT inline asm -- the
// project forbids hand-written asm outside the audited helpers).
// =============================================================================

#include "native/native_tuning.h"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <system_error>

#include "neoflux/core/config.h"

#include <glog/logging.h>

namespace neoflux::native {
namespace {

// Directory that holds index0..indexN for cpu0. Reading cpu0's tree is enough:
// the L1D coherence line size and the L2/L3 capacities are uniform across the
// package on every target NeoFlux supports (per-CPU asymmetry, e.g. hybrid
// parts with heterogeneous L2, is not something the alignment layer can act
// on anyway -- the ring-queue padding must pick one size).
constexpr const char* kCacheRoot = "/sys/devices/system/cpu/cpu0/cache";

/// Reads a small text file into `out` (NUL-terminated), trimming trailing
/// whitespace/newline. Returns false when the file cannot be opened.
bool ReadSysfsFile(const char* path, char* out, std::size_t out_size) {
  FILE* const fp = std::fopen(path, "r");
  if (fp == nullptr) {
    return false;
  }
  std::size_t n = std::fread(out, 1, out_size - 1, fp);
  std::fclose(fp);
  out[n] = '\0';
  // Trim trailing CR/LF/space so numeric parsing is stable.
  while (n > 0 && (out[n - 1] == '\n' || out[n - 1] == '\r' ||
                   out[n - 1] == ' ')) {
    out[--n] = '\0';
  }
  return true;
}

/// Parses a sysfs cache-size string such as "48K", "1280K", "16M", "2G" into
/// bytes. A bare number is treated as bytes. Returns 0 on anything unparsable.
///
/// The conversion uses std::from_chars, which is locale-independent: the sysfs
/// value is machine text, and std::strtoull would silently honour a program
/// that called setlocale(). The suffix table is a plain lookup rather than a
/// switch so that adding a unit is a one-line data change.
std::size_t ParseCacheSize(const char* text) {
  if (text == nullptr || *text == '\0') {
    return 0;
  }
  std::uint64_t value = 0;
  const char* const begin = text;
  const char* const end = text + std::strlen(text);
  const auto parsed = std::from_chars(begin, end, value);
  if (parsed.ec != std::errc{} || parsed.ptr == begin) {
    return 0;  // No leading digits, or out of range.
  }
  struct Unit {
    char letter;
    std::uint64_t multiplier;
  };
  // K/M/G are the only units the kernel ever writes; the lowercase forms are
  // accepted defensively because the field is plain text.
  static constexpr Unit kUnits[] = {
      {'K', 1024ULL},
      {'M', 1024ULL * 1024ULL},
      {'G', 1024ULL * 1024ULL * 1024ULL},
  };
  std::uint64_t multiplier = 1;  // Bare number: already bytes.
  const char suffix = *parsed.ptr;
  for (const Unit& unit : kUnits) {
    if (suffix == unit.letter || suffix == (unit.letter - 'A' + 'a')) {
      multiplier = unit.multiplier;
      break;
    }
  }
  return static_cast<std::size_t>(value * multiplier);
}

/// Reads one integer field (level / coherency_line_size) from an index dir.
/// Returns 0 on failure.
std::size_t ReadIndexInt(const std::string& index_dir, const char* field) {
  const std::string path = index_dir + "/" + field;
  char buf[64] = {};
  if (!ReadSysfsFile(path.c_str(), buf, sizeof(buf))) {
    return 0;
  }
  std::uint64_t value = 0;
  const char* const begin = buf;
  const char* const end = buf + std::strlen(buf);
  const auto parsed = std::from_chars(begin, end, value);
  if (parsed.ec != std::errc{} || parsed.ptr == begin) {
    return 0;
  }
  return static_cast<std::size_t>(value);
}

/// Reads a cache-size field at a given index dir (K/M/G aware). 0 on failure.
std::size_t ReadIndexSize(const std::string& index_dir) {
  const std::string path = index_dir + "/size";
  char buf[64] = {};
  if (!ReadSysfsFile(path.c_str(), buf, sizeof(buf))) {
    return 0;
  }
  return ParseCacheSize(buf);
}

/// True when `type` (as read from sysfs) counts as data-bearing: the unified
/// caches hold data as well as instructions, so they set L2/L3 sizes too.
bool IsDataBearing(const char* type) {
  return std::strcmp(type, "Data") == 0 || std::strcmp(type, "Unified") == 0;
}

}  // namespace

CacheInfo DetectCacheTopologySysfsImpl() noexcept {
  CacheInfo info;  // line_size = 64 by default -> the safe fallback.
  bool saw_l1d = false;

  // index0..indexN with no holes; stop at the first missing index dir.
  for (int index = 0; index < 16; ++index) {
    const std::string dir = std::string(kCacheRoot) + "/index" +
                            std::to_string(index);

    // level + type are mandatory; absence means we walked past the last index.
    const std::string type_path = dir + "/type";
    char type[32] = {};
    if (!ReadSysfsFile(type_path.c_str(), type, sizeof(type))) {
      break;
    }
    const std::size_t level = ReadIndexInt(dir, "level");
    if (level == 0) {
      continue;  // Malformed entry: skip rather than abort the whole walk.
    }

    if (level == 1 && std::strcmp(type, "Data") == 0) {
      // L1D coherence line size -- the number that governs false sharing.
      const std::size_t line = ReadIndexInt(dir, "coherency_line_size");
      if (line != 0) {
        info.line_size = line;
        saw_l1d = true;
      }
    }

    if (!IsDataBearing(type)) {
      continue;  // Instruction-only caches never set the capacity fields.
    }
    const std::size_t size = ReadIndexSize(dir);
    if (size == 0) {
      continue;
    }
    // Keep the largest data-bearing cache seen at each level (some SoCs list
    // multiple L1D/L2 banks; the biggest is the useful figure).
    if (level == 1) {
      info.l1d_bytes = size > info.l1d_bytes ? size : info.l1d_bytes;
    } else if (level == 2) {
      info.l2_bytes = size > info.l2_bytes ? size : info.l2_bytes;
    } else if (level == 3) {
      info.l3_bytes = size > info.l3_bytes ? size : info.l3_bytes;
    }
  }

  if (!saw_l1d) {
    // Container/VM without the cpu cache tree, or a kernel build that omits
    // it: keep line_size = 64 and all capacities 0 (unknown) -- documented
    // fallback behaviour.
    LOG(INFO) << "native: cache sysfs unavailable, cache line assumed 64B";
  }
  return info;
}

void PrefetchForRead(const void* p) noexcept {
  // locality = 3 (keep in cache), rw = 0 (read). Shared lines are never
  // modified, so the read form is correct for the immutable render assets.
  __builtin_prefetch(p, 0, 3);
}

void PrefetchForWrite(const void* p) noexcept {
  // rw = 1 asks for the line in the exclusive state, sparing the
  // read-for-ownership bus traffic before the store. On x86-64 GCC and Clang
  // lower this to PREFETCHW when the target enables PRFCHW, and to a plain
  // PREFETCHT0 otherwise; on AArch64 it is PRFM PSTL1KEEP. Either form is a
  // hint that cannot fault on an unmapped address.
  __builtin_prefetch(p, 1, 3);
}

void VerifyCacheLineConfig() noexcept {
  const CacheInfo info = DetectCacheTopology();
  LOG_FIRST_N(INFO, 1) << "native: cache topology detected -- line="
                       << info.line_size << "B L1d=" << info.l1d_bytes
                       << "B L2=" << info.l2_bytes << "B L3=" << info.l3_bytes
                       << "B";

  if (info.line_size > config::kCacheLineSize) {
    // The runtime coherence line is WIDER than the compile-time alignment:
    // objects padded to kCacheLineSize can still share a real cache line,
    // so false sharing is possible. Warn exactly once (LOG_FIRST_N) and tell
    // the user how to fix it.
    LOG_FIRST_N(WARNING, 1)
        << "native: runtime cache line (" << info.line_size
        << "B) exceeds compile-time config::kCacheLineSize ("
        << config::kCacheLineSize
        << "B); SPSC queue head/tail may still share a coherence line -> "
           "false sharing. Rebuild with -DNEOFLUX_CACHE_LINE_SIZE="
        << info.line_size << " to fix it.";
  } else if (info.line_size < config::kCacheLineSize) {
    // Oversized padding: correct but wasteful (a little memory), so info only.
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
  static const CacheInfo kCached = DetectCacheTopologySysfsImpl();
  return kCached;
}

}  // namespace neoflux::native
