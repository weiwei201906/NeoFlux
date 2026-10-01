// =============================================================================
// NeoFlux - card.h
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
// A Card is a surface panel: a rounded background that frames its content with
// a uniform inner padding. It is a Container preconfigured with a light
// surface color, corner radius, and padding.
//
// Pimpl: Card::Impl (card.cpp) holds implementation state. This header
// includes nothing but <memory> and the widget base.
// =============================================================================

#ifndef NEOFLUX_WIDGET_CARD_H_
#define NEOFLUX_WIDGET_CARD_H_

#include <memory>

#include "neoflux/widgets/container.h"

namespace neoflux {

// A rounded, padded surface panel that frames a single child.
class Card : public Container {
 public:
  explicit Card(std::shared_ptr<Widget> child = nullptr);
  ~Card() override;

  [[nodiscard]] std::string_view GetWidgetName() const noexcept override;

  // Sets the panel background color.
  Card& SetCardColor(const Color& color) noexcept;

  // Sets the corner radius in pixels.
  Card& SetCardRadius(float radius) noexcept;

  // Sets the uniform inner padding around the content.
  Card& SetCardPadding(float padding) noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace neoflux

#endif  // NEOFLUX_WIDGET_CARD_H_
