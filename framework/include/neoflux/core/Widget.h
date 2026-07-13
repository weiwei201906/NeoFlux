#pragma once

#include "BuildContext.h"
#include "neoflux/render/RenderContext.h"
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace neoflux {
namespace core {

class BuildContext;

class Widget {
 public:
  explicit Widget(std::string_view name = "widget");
  virtual ~Widget();

  void setBounds(int x, int y, int width, int height);
  virtual void layout(const BuildContext& context);
  virtual void render() const;
  virtual void render(const neoflux::render::RenderContext& context) const;
  void addChild(std::shared_ptr<Widget> child);

  template <typename T, typename... Args>
  std::shared_ptr<T> emplaceChild(Args&&... args) {
    auto child = std::make_shared<T>(std::forward<Args>(args)...);
    addChild(child);
    return child;
  }

  const std::vector<std::shared_ptr<Widget>>& children() const;
  const std::string& name() const;
  int x() const;
  int y() const;
  int width() const;
  int height() const;

 protected:
  std::string name_;
  int x_ = 0;
  int y_ = 0;
  int width_ = 0;
  int height_ = 0;
  std::vector<std::shared_ptr<Widget>> children_;
};

using WidgetPtr = std::shared_ptr<Widget>;

} // namespace core
} // namespace neoflux
