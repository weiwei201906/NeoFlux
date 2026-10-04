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

# ---------------------------------------------------------------------------
# Version: derive from the latest git tag (e.g. v0.3.1-alpha -> 0.3.1-alpha).
# Falls back to "0.0.0" when git is unavailable or has no tags, so configure
# never hard-fails on a shallow/exported checkout.
#
# CMake's project(VERSION) only accepts numeric components
# (major[.minor[.patch[.tweak]]]), so the full tag (which may carry a prerelease
# suffix such as "-alpha") is kept in NEOFLUX_VERSION for NEOFLUX_VERSION_STRING,
# while NEOFLUX_VERSION_NUMERIC is the sanitized form fed to project().
# ---------------------------------------------------------------------------
set(NEOFLUX_VERSION "0.0.0")
find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" describe --tags --abbrev=0
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE NEOFLUX_GIT_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE NEOFLUX_GIT_RESULT)
  if(NEOFLUX_GIT_RESULT EQUAL 0 AND NOT NEOFLUX_GIT_TAG STREQUAL "")
    string(REGEX REPLACE "^v" "" NEOFLUX_VERSION "${NEOFLUX_GIT_TAG}")
  endif()
endif()
# Keep only the leading numeric dotted components for project() (CMake rejects
# prerelease suffixes such as "-alpha").
string(REGEX MATCH "^[0-9]+(\\.[0-9]+)*" NEOFLUX_VERSION_NUMERIC "${NEOFLUX_VERSION}")
if(NEOFLUX_VERSION_NUMERIC STREQUAL "")
  set(NEOFLUX_VERSION_NUMERIC "0.0.0")
endif()
message(STATUS "NeoFlux version: ${NEOFLUX_VERSION} (from git describe --tags --abbrev=0)")

project(neoflux
  VERSION "${NEOFLUX_VERSION_NUMERIC}"
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
  src/views/media/media_view.cpp
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

# ---------------------------------------------------------------------------
# Copy runtime assets (fonts, media) next to the executable at build time.
# FontManager scans <cwd>/assets/fonts; this makes a fresh build find fonts
# whether you run from build/ or build/bin/.
# ---------------------------------------------------------------------------
add_custom_command(TARGET neoflux_app POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/assets
          $<TARGET_FILE_DIR:neoflux_app>/assets
  COMMENT "Copying runtime assets (fonts) next to neoflux_app"
)

# On Windows, the loader resolves libmpv-2.dll from the executable directory.
# Copy it next to neoflux_app when the prebuilt bundle exists AND the build
# targets the OpenGL backend -- media is bound to the GL backend module, so
# other backends neither need nor get the DLL.
if(WIN32
   AND TGFX_USE_OPENGL
   AND EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/mpv-bundle/libmpv-2.dll")
  add_custom_command(TARGET neoflux_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/thirdparty/mpv-bundle/libmpv-2.dll"
            "$<TARGET_FILE_DIR:neoflux_app>/libmpv-2.dll"
    COMMENT "Copying libmpv-2.dll next to neoflux_app"
  )
endif()

# ---------------------------------------------------------------------------
# Example applications (opt-in via -DNEOFLUX_BUILD_EXAMPLES=ON).
# ---------------------------------------------------------------------------
if(NEOFLUX_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
