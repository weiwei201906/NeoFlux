#pragma once

#include "../core/Widget.h"

namespace neoflux {
namespace widgets {

class ContainerWidget : public core::Widget {
 public:
  enum class Direction { Row, Column };

  explicit ContainerWidget(Direction direction = Direction::Column);
  void layout(const core::BuildContext& context) override;
  void render() const override;
  void render(const neoflux::render::RenderContext& context) const override;

 private:
  Direction direction_;
};

} // namespace widgets
} // namespace neoflux
