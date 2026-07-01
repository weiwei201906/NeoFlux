#pragma once

#include <memory>
#include <vector>

namespace neoflux {
namespace plugin {

class Plugin;

class PluginRegistry {
 public:
  void registerPlugin(std::shared_ptr<Plugin> plugin);
  void attachAll();
  void updateAll();
  void detachAll();

  const std::vector<std::shared_ptr<Plugin>>& plugins() const;

 private:
  std::vector<std::shared_ptr<Plugin>> plugins_;
};

} // namespace plugin
} // namespace neoflux
