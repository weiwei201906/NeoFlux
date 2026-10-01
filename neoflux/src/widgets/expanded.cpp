// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - expanded.cpp
//
// Implementation of Expanded.
// =============================================================================

#include "neoflux/widgets/expanded.h"

#include <utility>

namespace neoflux {

Expanded::Expanded(std::shared_ptr<Widget> child, int flex) {
  SetFlexGrow(static_cast<float>(flex > 0 ? flex : 1));
  if (child != nullptr) {
    AddChild(std::move(child));
  }
}

std::string_view Expanded::GetWidgetName() const noexcept {
  return "Expanded";
}

Expanded& Expanded::SetFlex(int flex) {
  SetFlexGrow(static_cast<float>(flex > 0 ? flex : 1));
  return *this;
}

}  // namespace neoflux
