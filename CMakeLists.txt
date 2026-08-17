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
option(NEOFLUX_BUILD_TESTS    "Build unit tests"         OFF)
option(NEOFLUX_BUILD_EXAMPLES "Build example applications" ON)
option(NEOFLUX_ENABLE_CLANG_TIDY "Enable clang-tidy as a build step (requires correct sysroot config)" OFF)
option(NEOFLUX_USE_TGFX      "Use tgfx rendering backend (requires MSVC on Windows)" OFF)

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
  src/core/font_manager.cpp
  src/core/types.cpp
  src/widget/widget.cpp
  src/widget/container.cpp
  src/widget/text.cpp
  src/widget/button.cpp
  src/widget/scroll_view.cpp
  src/widget/sized_box.cpp
  src/widget/expanded.cpp
  src/widget/draggable.cpp
  src/widget/route_registry.cpp
  src/app/application.cpp
  src/app/event_loop.cpp
  src/render/render_layer.cpp
  src/render/render_command.cpp
  src/render/render_context.cpp
  src/render/tgfx_renderer.cpp
)

# Platform-specific bridge implementations.
if(ANDROID OR (APPLE AND IOS))
  target_sources(neoflux PRIVATE src/render/mobile_bridge.cpp)
else()
  target_sources(neoflux PRIVATE src/render/glfw_bridge.cpp)
endif()

target_include_directories(neoflux
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_SOURCE_DIR}/thirdparty
)

# OpenGL is required for both the GLFW bridge and the OpenGL fallback renderer.
find_package(OpenGL REQUIRED)

target_link_libraries(neoflux
  PUBLIC
    taitank
    glog::glog
    gflags
    freetype
    OpenGL::GL
)

# tgfx is an optional rendering backend. When enabled, tgfx performs all
# rendering; when disabled, a built-in OpenGL renderer (shader + VBO +
# FreeType for UTF-8 text) is used. GLFW remains only a window/context
# bridge in both cases.
if(NEOFLUX_USE_TGFX AND TARGET tgfx)
  target_link_libraries(neoflux PRIVATE tgfx)
  target_compile_definitions(neoflux PRIVATE NEOFLUX_HAVE_TGFX=1)
  target_include_directories(neoflux PRIVATE
    ${tgfx_SOURCE_DIR}/src
    ${tgfx_SOURCE_DIR}/include
  )
endif()

# GLFW is only available on desktop platforms.
if(TARGET glfw)
  target_link_libraries(neoflux PRIVATE glfw)
endif()

target_compile_features(neoflux PUBLIC cxx_std_20)

# Strict warnings for neoflux code only (not third-party deps).
# -Wno-conversion/-Wno-sign-conversion: render code mixes int coords with
# size_t indices; unavoidable without excessive casts.
# -Wno-error=attributes: glog headers emit dllimport attribute warnings on
# Windows; warning is still shown, just not fatal.
target_compile_options(neoflux PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror -Wno-conversion -Wno-sign-conversion -Wno-error=attributes>
)

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
