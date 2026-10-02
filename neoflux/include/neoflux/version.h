// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - version.h
//
// Exposes the framework version as a string. The version is derived from the
// latest git tag at configure time (see root CMakeLists.txt) and baked into
// the binary via the NEOFLUX_VERSION_STRING compile definition.
// =============================================================================

#ifndef NEOFLUX_CORE_VERSION_H_
#define NEOFLUX_CORE_VERSION_H_

#include <string_view>

namespace neoflux {

// Returns the NeoFlux framework version (e.g. "0.3.0-alpha"), or "0.0.0" when
// built without version information. The returned view points to a static
// string literal and is valid for the lifetime of the process.
[[nodiscard]] std::string_view Version() noexcept;

}  // namespace neoflux

#endif  // NEOFLUX_CORE_VERSION_H_
