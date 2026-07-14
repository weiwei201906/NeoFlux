#include "DemoPlugin.h"

#include <utility>

#include <neoflux/core/Widget.h>
#include <neoflux/widgets/ButtonWidget.h>
#include <neoflux/widgets/TextWidget.h>

namespace neoflux {
namespace app {

DemoPlugin::DemoPlugin(std::string name) : neoflux::plugin::Plugin(std::move(name)) {}

DemoPlugin::~DemoPlugin() = default;

void DemoPlugin::setTarget(std::shared_ptr<core::Widget> target) {
  target_ = std::move(target);
}

void DemoPlugin::attach() {
  if (attached_ || !target_) {
    return;
  }

  auto summary = std::make_shared<neoflux::widgets::TextWidget>("plugin: attached");
  auto action = std::make_shared<neoflux::widgets::ButtonWidget>("plugin action");
  summary->setFlex(2U);
  action->setFlex(1U);
  target_->addChild(summary);
  target_->addChild(action);
  attached_ = true;
}

void DemoPlugin::update() {
  if (!attached_ || !target_) {
    return;
  }

  ++frames_;
  if (!frames_ & 1U) {
    auto* summary = dynamic_cast<neoflux::widgets::TextWidget*>(target_->children().back().get());
    if (summary != nullptr) {
      summary->setText("plugin: frame " + std::to_string(frames_));
    }
  }
}

void DemoPlugin::detach() {
  attached_ = false;
  target_.reset();
}

}  // namespace app
}  // namespace neoflux
