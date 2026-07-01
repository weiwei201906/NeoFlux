#include <atomic>
#include <chrono>
#include <glog/logging.h>
#include <thread>

namespace neoflux {
namespace platform {

class PlatformWindow {
 public:
  explicit PlatformWindow(std::string title = "NeoFlux Window") : title_(std::move(title)) {}
  ~PlatformWindow() = default;

  void open() {
    if (isOpen_) {
      return;
    }
    isOpen_ = true;
    LOG(INFO) << "PlatformWindow: opening " << title_;
  }

  void close() {
    if (!isOpen_) {
      return;
    }
    isOpen_ = false;
    LOG(INFO) << "PlatformWindow: closing " << title_;
  }

  bool isOpen() const {
    return isOpen_;
  }

 private:
  std::string title_;
  std::atomic<bool> isOpen_ = false;
};

} // namespace platform
} // namespace neoflux
