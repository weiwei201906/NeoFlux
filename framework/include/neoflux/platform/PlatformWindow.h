#pragma once

#include <atomic>
#include <memory>
#include <string>

namespace neoflux::platform {

class PlatformWindow {
 public:
  explicit PlatformWindow(std::string title = "NeoFlux Window");
  ~PlatformWindow();

  void open();
  void close();
  [[nodiscard]] bool isOpen() const;
  bool pollEvents();
  void* nativeHandle() const;

 private:
  struct Impl;

  std::string title_;
  std::atomic<bool> isOpen_ = false;
  std::unique_ptr<Impl> impl_;
};

}  // namespace neoflux::platform