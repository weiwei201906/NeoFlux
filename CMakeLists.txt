# =============================================================================
# NeoFlux - host project skeleton (user applications)
#
# This top-level CMakeLists is the "quick-start" project for building your own
# NeoFlux application directly on top of a clone of this repository:
#
#   1. thirdparty/  - third-party dependencies (git submodules)
#   2. neoflux/     - the framework library (also a git submodule)
#   3. src/         - YOUR application code (see src/main.cpp)
#
# To build:  cmake -B build && cmake --build build
# To test:   cmake -B build -DNEOFLUX_BUILD_TESTS=ON && ctest --test-dir build
#
# The framework is consumed via add_subdirectory(neoflux), so you can also
# vendor only the neoflux/ directory into your own project and link the
# `neoflux` target directly.
#
# Adding a new source file to your app: list it under add_executable below
# (e.g. src/views/home_view.cpp, src/router/index.cpp).
# =============================================================================

cmake_minimum_required(VERSION 3.20)

project(neoflux_app
  VERSION 0.1.0
  DESCRIPTION "NeoFlux application quick-start project"
  LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Allow old third-party CMakeLists (e.g. gflags 2.2.2 requires CMake < 3.5
# compatibility which was removed in CMake 4.x).
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# ---------------------------------------------------------------------------
# Third-party dependencies (git submodules under thirdparty/)
# ---------------------------------------------------------------------------
add_subdirectory(thirdparty)

add_subdirectory(neoflux)

# ---------------------------------------------------------------------------
# Your application
# ---------------------------------------------------------------------------
# Keep application sources grouped so new files are easy to add. Views live
# under src/views/ (one view per route); shared reusable widgets live under
# src/widgets/; routes are registered centrally in src/router/index.cpp.
set(QUICKSTART_SOURCES
  src/main.cpp
  src/router/index.cpp
  src/widgets/back_button.cpp
  src/views/home_view.cpp
  src/views/counter_view.cpp
  src/views/about_view.cpp
)

# Name of the quick-start executable. Override it for your own project with
#   cmake -DNEOFLUX_APP_NAME=my_app
set(NEOFLUX_APP_NAME "neoflux_app" CACHE STRING
    "Name of the quick-start application executable")

add_executable(${NEOFLUX_APP_NAME} ${QUICKSTART_SOURCES})
target_link_libraries(${NEOFLUX_APP_NAME} PRIVATE neoflux)
target_include_directories(${NEOFLUX_APP_NAME} PRIVATE src)
