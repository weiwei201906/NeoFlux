#pragma once

#include <memory>
#include <string>

#include <neoflux/plugin/Plugin.h>

namespace neoflux {
namespace app {

class DemoPlugin : public neoflux::plugin::Plugin {
 public:
  explicit DemoPlugin(std::string name = "demo-plugin");
  ~DemoPlugin() override;

  void setTarget(std::shared_ptr<core::Widget> target);
  void attach() override;
  void update() override;
  void detach() override;

 private:
  std::shared_ptr<core::Widget> target_;
  std::size_t frames_ = 0U;
  bool attached_ = false;
};

}  // namespace app
}  // namespace neoflux
