// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - native/cache_topology_cpuid.h
//
// x86 cache-topology detection through CPUID, shared by every x86 platform.
//
// Windows and Linux used to carry a near-identical copy of this walk, one
// calling __cpuid/__cpuidex and the other __get_cpuid_count. Two copies of a
// bit-field decoder is two chances to transcribe a mask wrong, and the one
// transcribed wrong is the one nobody runs. There is now one decoder, and it
// goes through cpuid_bits.h so the register read itself is also shared.
//
// Detection strategy (Intel SDM Vol. 2A, CPUID leaf 04H)
// ------------------------------------------------------
//   * Leaf 1, EBX[15:8]  CLFLUSH line size in 8-byte units. Used as the
//     fallback line size, and as a cross-check when a cache reports nonsense.
//   * Leaf 4 subleaf 0,1,2,... until EAX[4:0] (cache type) reads 0:
//       type            EAX[4:0]       1 = data, 2 = instruction, 3 = unified
//       level           EAX[7:5]
//       line_size       EBX[11:0]  + 1  threads sharing the cache
//       partitions      EBX[21:12] + 1  physical line partitions
//       ways            EBX[31:22] + 1  associativity
//       sets            ECX        + 1
//       total_size      ways * partitions * line_size * sets
//   * Only data-bearing caches (type 1 data or 3 unified) feed the capacities.
//   * The walk stops at the first type of 0, at an unsupported leaf 4, and at
//     a hard cap of 16 subleaves so a malformed response cannot spin.
//
// This file is x86-only. AArch64 has no CPUID; Apple Silicon and ARM servers
// report the same figures through sysctl, and Android/Linux through sysfs, so
// those platforms keep their own probe.
// =============================================================================

#pragma once

#include <cstddef>
#include <cstdint>

#include "native/cpuid_bits.h"
#include "native/native_tuning.h"

namespace neoflux::native {

/// Walks CPUID leaf 4 and returns the coherence line size plus the L1d/L2/L3
/// capacities. Returns the documented fallback (line_size = 64, capacities 0 =
/// unknown) when CPUID is unavailable or reports nothing usable, so the result
/// is always safe to use for alignment decisions.
[[nodiscard]] inline CacheInfo DetectCacheTopologyCpuid() noexcept {
  CacheInfo info;  // line_size = 64 by default -> the safe fallback.

  const auto leaf0 = cpuid_bits::Subleaf(0U, 0U);
  const std::uint32_t max_leaf = leaf0[0];
  if (max_leaf < 1U) {
    return info;  // Should be impossible on any x86 that boots this code.
  }

  // Leaf 1: EBX[15:8] = CLFLUSH line size in 8-byte units.
  const auto leaf1 = cpuid_bits::Subleaf(1U, 0U);
  const std::uint32_t clflush_units = (leaf1[1] >> 8U) & 0xFFU;
  if (clflush_units != 0U) {
    info.line_size = static_cast<std::size_t>(clflush_units) * 8U;
  }

  if (max_leaf < 4U) {
    // Pre-Nehalem CPU: no deterministic-cache-parameter leaf. The CLFLUSH line
    // size (or the 64B default) still stands; capacities stay 0 = unknown.
    return info;
  }

  // Cache-type encodings from EAX[4:0]. The field is five bits wide, so a
  // one-byte underlying type is all the value can ever need.
  enum CacheType : std::uint8_t {
    kTypeNone = 0,         // "no more caches" sentinel: leaves the loop.
    kTypeData = 1,         // data cache
    kTypeInstruction = 2,  // instruction cache
    kTypeUnified = 3,      // unified (data + instruction)
  };

  // Defensive cap: a well-formed CPU reports well under 16 subleaves.
  for (std::uint32_t subleaf = 0U; subleaf < 16U; ++subleaf) {
    const auto leaf4 = cpuid_bits::Subleaf(4U, subleaf);
    const auto type = static_cast<CacheType>(leaf4[0] & 0x1FU);
    if (type == kTypeNone) {
      break;  // No further caches reported.
    }
    const std::uint32_t level = (leaf4[0] >> 5U) & 0x7U;

    const std::size_t line_size =
        static_cast<std::size_t>(leaf4[1] & 0x0FFFU) + 1U;          // EBX[11:0]
    const std::size_t partitions =
        static_cast<std::size_t>((leaf4[1] >> 12U) & 0x03FFU) + 1U;  // EBX[21:12]
    const std::size_t ways =
        static_cast<std::size_t>((leaf4[1] >> 22U) & 0x03FFU) + 1U;  // EBX[31:22]
    const std::size_t sets = static_cast<std::size_t>(leaf4[2]) + 1U;  // ECX
    const std::size_t total = ways * partitions * line_size * sets;

    // Prefer the L1 data cache's own line size over the CLFLUSH figure: it is
    // the coherence line of the cache that actually holds the SPSC queue
    // head/tail, which is what the alignment decision depends on.
    if (level == 1U && type == kTypeData && line_size != 0U) {
      info.line_size = line_size;
    }

    const bool data_bearing = type == kTypeData || type == kTypeUnified;
    if (!data_bearing) {
      continue;  // Instruction-only caches do not contribute capacity.
    }
    if (level == 1U) {
      info.l1d_bytes = total > info.l1d_bytes ? total : info.l1d_bytes;
    } else if (level == 2U) {
      info.l2_bytes = total > info.l2_bytes ? total : info.l2_bytes;
    } else if (level == 3U) {
      info.l3_bytes = total > info.l3_bytes ? total : info.l3_bytes;
    }
  }

  return info;
}

}  // namespace neoflux::native
