# =============================================================================
# NeoFlux - A cross-platform C++20 UI framework
#
# Two-layer architecture:
#   - Application layer: business logic + Taitank layout engine
#   - Render layer: tgfx (mobile) / GLFW bridge (desktop)
#
# Communication: SPSC lock-free ring queue (FIFO)
#
# Repository layout after restructure:
#   neoflux/   the framework STATIC library (render, layout, events, routing,
#              widget base, mpv wrapper). Public headers under
#              neoflux/include/neoflux.
#   src/       the USER quick-start business project. A fresh clone lets you
#              immediately edit src/ to build your own app against `neoflux`.
# =============================================================================

cmake_minimum_required(VERSION 3.20)

project(neoflux
  VERSION 0.1.0
  DESCRIPTION "Cross-platform C++20 UI framework with Flutter-like widget model"
  LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default build type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Position-independent code for shared lib linkage
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(NEOFLUX_BUILD_TESTS    "Build framework unit tests"      OFF)
option(NEOFLUX_BUILD_EXAMPLES "Build example applications"       OFF)
option(NEOFLUX_ENABLE_CLANG_TIDY "Enable clang-tidy as a build step (requires correct sysroot config)" OFF)
option(NEOFLUX_USE_TGFX       "Use tgfx rendering backend (requires MSVC on Windows)" OFF)

# ---------------------------------------------------------------------------
# Third-party dependencies
# ---------------------------------------------------------------------------
# Allow old third-party CMakeLists (e.g. gflags 2.2.2 requires CMake < 3.5
# compatibility which was removed in CMake 4.x).
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
add_subdirectory(thirdparty)

# ---------------------------------------------------------------------------
# Compiler flags (applied to targets created below, i.e. framework + user app,
# but NOT to the third-party dependencies added just above).
# ---------------------------------------------------------------------------
include(cmake/CompilerFlags.cmake)

# ---------------------------------------------------------------------------
# clang-tidy integration
# ---------------------------------------------------------------------------
if(NEOFLUX_ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy clang-tidy-18 clang-tidy-17 clang-tidy-16)
  if(CLANG_TIDY_EXE)
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
    set(CMAKE_CXX_CLANG_TIDY
      "${CLANG_TIDY_EXE}"
      "-config-file=${CMAKE_SOURCE_DIR}/.clang-tidy"
    )
  else()
    message(STATUS "clang-tidy not found - skipping static analysis")
  endif()
endif()

# ---------------------------------------------------------------------------
# Framework library (neoflux STATIC target).
# ---------------------------------------------------------------------------
if(NEOFLUX_BUILD_TESTS)
  enable_testing()
endif()
add_subdirectory(neoflux)

# ---------------------------------------------------------------------------
# User quick-start application (top-level src/).
#
# This is YOUR app: edit src/views, src/routers, and src/main.cpp. It links the
# `neoflux` framework library and inherits its public include directories.
# ---------------------------------------------------------------------------
add_executable(neoflux_app
  src/main.cpp
  src/routers/index.cpp
  src/views/home/home_view.cpp
)

target_include_directories(neoflux_app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(neoflux_app
  PRIVATE
    neoflux
    glog::glog
    gflags
)

target_compile_features(neoflux_app PRIVATE cxx_std_20)
