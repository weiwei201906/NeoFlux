#pragma once

#include <string>

namespace neoflux::plugin {

class Plugin {
 public:
  explicit Plugin(std::string name = "plugin");
  virtual ~Plugin();

  virtual void attach() = 0;
  virtual void update() = 0;
  virtual void detach() = 0;

  const std::string& name() const;

 protected:
  std::string name_;
};

}  // namespace neoflux::plugin