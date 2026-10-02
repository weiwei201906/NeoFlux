// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - version.cpp
// =============================================================================

#include "neoflux/version.h"

namespace neoflux {

std::string_view Version() noexcept {
#ifdef NEOFLUX_VERSION_STRING
  return NEOFLUX_VERSION_STRING;
#else
  return "0.0.0";
#endif
}

}  // namespace neoflux
