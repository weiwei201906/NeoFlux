// =============================================================================
// NeoFlux - sized_box.cpp
//
// Implementation of SizedBox.
// =============================================================================

#include "neoflux/widget/sized_box.h"

#include <utility>

namespace neoflux {

SizedBox::SizedBox() = default;

SizedBox::SizedBox(float width, float height) {
  if (width > 0.0F) {
    SetWidth(width);
  }
  if (height > 0.0F) {
    SetHeight(height);
  }
}

std::string_view SizedBox::GetWidgetName() const noexcept {
  return "SizedBox";
}

SizedBox& SizedBox::SetWidth(float width) {
  if (width > 0.0F) {
    Container::SetWidth(width);
  }
  return *this;
}

SizedBox& SizedBox::SetHeight(float height) {
  if (height > 0.0F) {
    Container::SetHeight(height);
  }
  return *this;
}

SizedBox& SizedBox::SetSize(float width, float height) {
  SetWidth(width);
  SetHeight(height);
  return *this;
}

}  // namespace neoflux
