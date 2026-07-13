#pragma once

#include "../core/Widget.h"

#include <functional>
#include <string>
#include <string_view>

namespace neoflux {
namespace widgets {

class ButtonWidget : public core::Widget {
 public:
  explicit ButtonWidget(std::string_view title = "Button");

  void setOnClick(std::function<void()> callback);
  const std::string& title() const;
  void layout(const core::BuildContext& context) override;
  void render() const override;

 private:
  std::string title_;
  std::function<void()> onClick_;
};

} // namespace widgets
} // namespace neoflux
