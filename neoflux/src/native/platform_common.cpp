// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - platform_common.cpp
//
// Fallback for platforms without a dedicated tuning implementation
// (BSDs, exotic/embedded unixes). Everything degrades to a safe no-op.
// =============================================================================

#include "native/native_tuning.h"

namespace neoflux {
namespace native {

void TuneRenderThread() {}

void TuneUiThread() {}

void PinThreadToBigCores() {}

CpuFeatures DetectCpuFeatures() { return {}; }

}  // namespace native
}  // namespace neoflux
