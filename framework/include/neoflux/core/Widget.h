#pragma once

#include "BuildContext.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
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
  void addChild(std::shared_ptr<Widget> child);

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
