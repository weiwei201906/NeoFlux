#include "neoflux/platform/PlatformWindow.h"

#include <glog/logging.h>
#include <utility>

#if defined(__linux__) && __has_include(<X11/Xlib.h>)
#include <X11/Xlib.h>
#endif

namespace neoflux::platform {

struct PlatformWindow::Impl {
#if defined(__linux__) && __has_include(<X11/Xlib.h>)
  Display* display = nullptr;
  Window window = 0;
#endif
};

PlatformWindow::PlatformWindow(std::string title) : title_(std::move(title)), impl_(std::make_unique<Impl>()) {}

PlatformWindow::~PlatformWindow() { close(); }

void PlatformWindow::open() {
  if (isOpen_.load()) {
    return;
  }

#if defined(__linux__) && __has_include(<X11/Xlib.h>)
  impl_->display = XOpenDisplay(nullptr);
  if (impl_->display != nullptr) {
    const int screen = DefaultScreen(impl_->display);
    impl_->window = XCreateSimpleWindow(impl_->display, RootWindow(impl_->display, screen), 0, 0, 800, 600, 1,
                                        BlackPixel(impl_->display, screen), WhitePixel(impl_->display, screen));
    XStoreName(impl_->display, impl_->window, title_.c_str());
    XMapWindow(impl_->display, impl_->window);
    XFlush(impl_->display);
    LOG(INFO) << "PlatformWindow: opened X11 window '" << title_ << "'";
  } else {
    LOG(WARNING) << "PlatformWindow: X11 display unavailable, running in headless mode for '" << title_ << "'";
  }
#else
  LOG(INFO) << "PlatformWindow: opening virtual window '" << title_ << "'";
#endif

  isOpen_.store(true);
}

void PlatformWindow::close() {
  if (!isOpen_.load()) {
    return;
  }

#if defined(__linux__) && __has_include(<X11/Xlib.h>)
  if (impl_->display != nullptr && impl_->window != 0) {
    XDestroyWindow(impl_->display, impl_->window);
    XCloseDisplay(impl_->display);
    impl_->display = nullptr;
    impl_->window = 0;
  }
#endif

  isOpen_.store(false);
  LOG(INFO) << "PlatformWindow: closed '" << title_ << "'";
}

bool PlatformWindow::isOpen() const { return isOpen_.load(); }

}  // namespace neoflux::platform