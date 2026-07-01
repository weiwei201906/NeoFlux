#pragma once

#include <memory>

namespace neoflux {
namespace core {
class Widget;
}
}

std::shared_ptr<neoflux::core::Widget> createAppWidget();
