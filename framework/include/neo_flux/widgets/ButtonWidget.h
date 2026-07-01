#pragma once

#include "../core/Widget.h"

#include <functional>
#include <string>

namespace neoflux {
namespace widgets {

class ButtonWidget : public core::Widget {
 public:
  explicit ButtonWidget(std::string title = "Button");

  void setOnClick(std::function<void()> callback);
  void layout(const core::BuildContext& context) override;
  void render() const override;

 private:
  std::string title_;
  std::function<void()> onClick_;
};

} // namespace widgets
} // namespace neoflux
