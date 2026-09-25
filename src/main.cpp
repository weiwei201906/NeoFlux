// =============================================================================
// NeoFlux - quick-start application entry point (src/main.cpp)
//
// Minimal host application skeleton. The framework library lives in neoflux/;
// this directory (src/) is YOUR project. Organize it however you like -- the
// layout below is a suggested starting point:
//
//   src/
//   ├── main.cpp          <-- you are here
//   ├── router/
//   │   ├── index.h       <-- RegisterRoutes() declaration
//   │   └── index.cpp     <-- central route registration
//   └── views/            <-- one view per route (widgets live here, not in
//                             a top-level widget/ directory)
//
// Build & run:
//   cmake -B build
//   cmake --build build
//   ./build/bin/neoflux_app
//
// Notes:
//   - Register ALL routes (even a single "/" route) before Init().
//   - SetFontDir() must be called before Init(); the build copies fonts to
//     ./assets/fonts/ (see neoflux/CMakeLists.txt).
// =============================================================================

#include "router/index.h"

#include <neoflux/app/application.h>

#include <glog/logging.h>
#include <glog/raw_logging.h>

int main(int argc, char** argv) {
  using neoflux::Application;

  // Every framework call below may throw (e.g. std::bad_alloc). Both catches
  // keep exceptions from escaping main, so a NOLINT for the conservative
  // bugprone-exception-escape check is justified here.
  try {  // NOLINT(bugprone-exception-escape): all exceptions are caught below
    // Register every route up front. See router/index.cpp for the list.
    RegisterRoutes();

    Application app;
    app.SetFontDir("./assets/fonts/");
    if (!app.Init(argc, argv, 1200, 600, "NeoFlux Quick Start")) {
      LOG(ERROR) << "Failed to initialize application";
      return 1;
    }

    app.PushRoute("/");
    app.Run();
    return 0;
  } catch (const std::exception& ex) {
    // RAW_LOG is exception-free; LOG(ERROR) could itself throw here.
    RAW_LOG(ERROR, "Unhandled exception in main: %s", ex.what());  // NOLINT(cppcoreguidelines-avoid-do-while): glog macro
    return 1;
  } catch (...) {
    RAW_LOG(ERROR, "Unhandled non-standard exception in main");  // NOLINT(cppcoreguidelines-avoid-do-while): glog macro
    return 1;
  }
}
