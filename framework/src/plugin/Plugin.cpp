#include "neoflux/plugin/Plugin.h"

namespace neoflux {
namespace plugin {

Plugin::Plugin(std::string name) : name_(std::move(name)) {}

Plugin::~Plugin() = default;

const std::string& Plugin::name() const { return name_; }

}  // namespace plugin
}  // namespace neoflux
