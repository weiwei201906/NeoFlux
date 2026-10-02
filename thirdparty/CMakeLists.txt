# =============================================================================
# Third-party dependencies for NeoFlux
#
# Every dependency is vendored under thirdparty/ as a git submodule. We add
# each one directly with add_subdirectory so:
#   1. A fresh clone (with --recurse-submodules) builds fully offline.
#   2. The exact pinned revision is reviewable in this repository.
#   3. No CMake FetchContent cache lives in _deps/ (which confused CI and
#      left stale builds after option changes).
# =============================================================================

# Allow old third-party CMakeLists (e.g. gflags 2.2.2 requires CMake < 3.5
# compatibility which was removed in CMake 4.x).
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# ---------------------------------------------------------------------------
# Google glog - logging library
# ---------------------------------------------------------------------------
set(WITH_GFLAGS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(glog)

# ---------------------------------------------------------------------------
# Google gflags - command-line flag parsing
# ---------------------------------------------------------------------------
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(gflags)

# ---------------------------------------------------------------------------
# Google Test - unit testing framework
# ---------------------------------------------------------------------------
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
add_subdirectory(googletest)

# ---------------------------------------------------------------------------
# GLFW - desktop window / input management (desktop only)
# ---------------------------------------------------------------------------
if(NOT ANDROID AND NOT IOS)
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
  add_subdirectory(glfw)
endif()

# ---------------------------------------------------------------------------
# Taitank - Tencent cross-platform layout engine (flexbox)
# ---------------------------------------------------------------------------
add_subdirectory(taitank)

# ---------------------------------------------------------------------------
# FreeType - TrueType/OpenType font rasterization (used by tgfx's text stack)
# ---------------------------------------------------------------------------
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
set(FT_ENABLE_ERROR_STRINGS OFF CACHE BOOL "" FORCE)
add_subdirectory(freetype)

# ---------------------------------------------------------------------------
# tgfx - Tencent lightweight 2D graphics rendering library (required backend)
#
# Vendored as a git repository under thirdparty/tgfx/ with its own submodules
# already synced (shaderc, glslang, etc.).
#
# NOTE: tgfx's CMakeLists unconditionally checks for shaderc/third_party/glslang
# at configure time and runs git-sync-deps (network) if missing. We use the
# OpenGL backend which does NOT need shaderc, so we drop an empty placeholder
# directory to skip the network sync on CI.
# ---------------------------------------------------------------------------
# tgfx backend selection (compile-time). Pick exactly one:
#   -DNEOFLUX_BACKEND=gl       (default, desktop + mobile OpenGL/EGL)
#   -DNEOFLUX_BACKEND=vulkan   (requires Vulkan-capable host + shaderc)
#   -DNEOFLUX_BACKEND=d3d12    (Windows only)
#   -DNEOFLUX_BACKEND=metal    (macOS only)
# ---------------------------------------------------------------------------
set(NEOFLUX_BACKEND "gl" CACHE STRING "tgfx rendering backend")
set_property(CACHE NEOFLUX_BACKEND PROPERTY STRINGS gl vulkan d3d12 metal)

set(TGFX_USE_OPENGL OFF)
set(TGFX_USE_D3D12 OFF)
set(TGFX_USE_METAL OFF)
set(TGFX_USE_VULKAN OFF)
set(TGFX_USE_WEBGPU OFF)

if(NEOFLUX_BACKEND STREQUAL "gl")
  set(TGFX_USE_OPENGL ON)
elseif(NEOFLUX_BACKEND STREQUAL "vulkan")
  set(TGFX_USE_VULKAN ON)
elseif(NEOFLUX_BACKEND STREQUAL "d3d12")
  set(TGFX_USE_D3D12 ON)
elseif(NEOFLUX_BACKEND STREQUAL "metal")
  set(TGFX_USE_METAL ON)
else()
  message(FATAL_ERROR "Unknown NEOFLUX_BACKEND='${NEOFLUX_BACKEND}'. Choose: gl, vulkan, d3d12, metal.")
endif()

set(TGFX_USE_FREETYPE ON CACHE BOOL "" FORCE)
set(TGFX_BUILD_SVG OFF CACHE BOOL "" FORCE)
set(TGFX_BUILD_PDF OFF CACHE BOOL "" FORCE)
set(TGFX_BUILD_LAYERS OFF CACHE BOOL "" FORCE)
set(TGFX_BUILD_HELLO2D OFF CACHE BOOL "" FORCE)

# Propagate the chosen backend to our sources.
add_compile_definitions(NEOFLUX_BACKEND_${NEOFLUX_BACKEND})

# Placeholder to skip tgfx's unconditional shaderc glslang network sync.
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tgfx/third_party/shaderc/third_party/glslang")
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tgfx/third_party/shaderc/third_party/glslang")
endif()
# tgfx includes third_party/vendor_tools/vendor.cmake which defines
# add_vendor_target / find_vendor_libraries. That file is only present when
# sync_tgfx_vendor.sh has been run locally; on CI it is absent. The OpenGL
# backend does not use tgfx's vendored third-party targets (we build freetype
# ourselves above), so we drop a stub that defines no-op functions.
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tgfx/third_party/vendor_tools/vendor.cmake")
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tgfx/third_party/vendor_tools")
  file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/tgfx/third_party/vendor_tools/vendor.cmake"
"# Stub vendor.cmake for CI: OpenGL backend does not need tgfx's vendor system.
function(add_vendor_target)
endfunction()
function(find_vendor_libraries)
  set(\${ARGV1} \"\" PARENT_SCOPE)
  set(\${ARGV3} \"\" PARENT_SCOPE)
endfunction()
")
endif()
add_subdirectory(tgfx)

message(STATUS "NeoFlux third-party dependencies configured")
