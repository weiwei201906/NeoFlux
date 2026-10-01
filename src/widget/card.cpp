// =============================================================================
// NeoFlux - card.cpp
//
// NOTICE: This widget was designed with reference to the Card component of
// EUI-NEO (https://github.com/sudoevolve/EUI-NEO), Copyright sudoevolve,
// licensed under the Apache License, Version 2.0. The component's structure
// (a rounded background panel that insets its content by a uniform padding)
// and default metrics were adapted to the NeoFlux widget/render model. No
// EUI-NEO source code was copied.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy
// of the License at http://www.apache.org/licenses/LICENSE-2.0
//
// Implementation of Card. All layout is delegated to Taitank via the Container
// base; this file only preconfigures the surface color, corner radius, and
// inner padding.
// =============================================================================

#include "neoflux/widget/card.h"

#include <utility>

namespace neoflux {

namespace {
// Default light surface color and metrics, matching a raised Material card.
constexpr Color kSurfaceColor{.r = 0xFA, .g = 0xFA, .b = 0xFA, .a = 0xFF};
constexpr float kDefaultRadius = 12.0F;
constexpr float kDefaultPadding = 16.0F;
}  // namespace

Card::Card(std::shared_ptr<Widget> child) {
  SetBackgroundColor(kSurfaceColor);
  SetBorderRadius(kDefaultRadius);
  SetCardPadding(kDefaultPadding);
  if (child != nullptr) {
    SetChild(std::move(child));
  }
}

std::string_view Card::GetWidgetName() const noexcept { return "Card"; }

Card& Card::SetCardColor(const Color& color) noexcept {
  SetBackgroundColor(color);
  return *this;
}

Card& Card::SetCardRadius(float radius) noexcept {
  SetBorderRadius(radius);
  return *this;
}

Card& Card::SetCardPadding(float padding) noexcept {
  SetPadding({.left = padding, .top = padding, .right = padding,
              .bottom = padding});
  return *this;
}

}  // namespace neoflux
