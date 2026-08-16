# =============================================================================
# NeoFlux - A cross-platform C++20 UI framework
#
# Two-layer architecture:
#   - Application layer: business logic + Taitank layout engine
#   - Render layer: tgfx (mobile) / GLFW bridge (desktop)
#
# Communication: SPSC lock-free ring queue (FIFO)
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
option(NEOFLUX_BUILD_TESTS    "Build unit tests"         ON)
option(NEOFLUX_BUILD_EXAMPLES "Build example applications" ON)
option(NEOFLUX_ENABLE_CLANG_TIDY "Enable clang-tidy checks" ON)
option(NEOFLUX_USE_TGFX      "Enable tgfx rendering backend (requires depsync + full tgfx deps)" OFF)

# ---------------------------------------------------------------------------
# Third-party dependencies
# ---------------------------------------------------------------------------
# Allow old third-party CMakeLists (e.g. gflags 2.2.2 requires CMake < 3.5
# compatibility which was removed in CMake 4.x).
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
add_subdirectory(thirdparty)

# ---------------------------------------------------------------------------
# Compiler flags
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
# Core library
# ---------------------------------------------------------------------------
add_library(neoflux STATIC
  src/core/ring_queue.cpp
  src/core/types.cpp
  src/widget/widget.cpp
  src/widget/container.cpp
  src/widget/text.cpp
  src/widget/button.cpp
  src/widget/route_registry.cpp
  src/app/application.cpp
  src/app/event_loop.cpp
  src/render/render_layer.cpp
  src/render/render_command.cpp
  src/render/render_context.cpp
  src/render/tgfx_renderer.cpp
  src/render/glfw_bridge.cpp
)

target_include_directories(neoflux
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(neoflux
  PUBLIC
    taitank
    glog::glog
    gflags
)

# tgfx is optional; when disabled, tgfx_renderer.cpp compiles a stub via
# __has_include detection.
if(NEOFLUX_USE_TGFX AND TARGET tgfx)
  target_link_libraries(neoflux PUBLIC tgfx)
  target_compile_definitions(neoflux PUBLIC NEOFLUX_HAVE_TGFX=1)
endif()

# GLFW is only available on desktop platforms.
if(TARGET glfw)
  target_link_libraries(neoflux PRIVATE glfw)
endif()

target_compile_features(neoflux PUBLIC cxx_std_20)

# Platform-specific definitions
if(WIN32)
  target_compile_definitions(neoflux PUBLIC NEOFLUX_PLATFORM_WINDOWS=1)
elseif(APPLE)
  target_compile_definitions(neoflux PUBLIC NEOFLUX_PLATFORM_MACOS=1)
elseif(UNIX AND NOT APPLE)
  target_compile_definitions(neoflux PUBLIC NEOFLUX_PLATFORM_LINUX=1)
endif()

if(ANDROID OR IOS)
  target_compile_definitions(neoflux PUBLIC NEOFLUX_PLATFORM_MOBILE=1)
else()
  target_compile_definitions(neoflux PUBLIC NEOFLUX_PLATFORM_DESKTOP=1)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(NEOFLUX_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# Examples
# ---------------------------------------------------------------------------
if(NEOFLUX_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# ---------------------------------------------------------------------------
# Install rules
# ---------------------------------------------------------------------------
install(TARGETS neoflux
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/neoflux DESTINATION include)
