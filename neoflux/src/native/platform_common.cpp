// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - platform_common.cpp
//
// Fallback for platforms without a dedicated tuning implementation
// (BSDs, exotic/embedded unixes). Everything degrades to a safe no-op.
// =============================================================================

#include "native/native_tuning.h"

namespace neoflux::native {

void TuneRenderThread() noexcept {}

void TuneUiThread() noexcept {}

void PinThreadToBigCores() noexcept {}

CpuFeatures DetectCpuFeatures() noexcept { return {}; }

CacheInfo DetectCacheTopology() noexcept {
  return {};  // line_size=64 (safe default), capacities 0 = unknown.
}

void PrefetchForRead(const void* p) noexcept { (void)p; }

void PrefetchForWrite(const void* p) noexcept { (void)p; }

void VerifyCacheLineConfig() noexcept {}

}  // namespace neoflux::native
