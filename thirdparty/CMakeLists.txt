# =============================================================================
# Third-party dependencies for NeoFlux
#
# All dependencies are Git submodules under thirdparty/ (see .gitmodules).
# Clone them once with:
#   git submodule update --init --recursive
# Each submodule is added via add_subdirectory so its targets (glog::glog,
# gflags, gtest, glfw, taitank, freetype, tgfx) are available to the
# framework and to host applications.
# =============================================================================

# ---------------------------------------------------------------------------
# Google glog - logging library
#
# WITH_GFLAGS must be ON: the framework reads glog's logtostderr / log_dir
# flags directly (they are then provided by the gflags library, whose target
# is linked below). With gflags disabled, glog keeps those symbols internal
# and linking the framework fails with unresolved externals.
# ---------------------------------------------------------------------------
set(WITH_GFLAGS ON CACHE BOOL "" FORCE)
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
# GLFW - desktop window / input management
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
# FreeType - high-quality TrueType/OpenType font rendering
#
# Used by the OpenGL fallback renderer for UTF-8 text rendering. Replaces
# stb_truetype which produced garbled glyphs with CJK fonts.
# ---------------------------------------------------------------------------
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
set(FT_ENABLE_ERROR_STRINGS OFF CACHE BOOL "" FORCE)
add_subdirectory(freetype)

# ---------------------------------------------------------------------------
# tgfx - Tencent lightweight 2D graphics rendering library (optional)
#
# tgfx uses `depsync` (npm package) to manage vendor dependencies, and its
# vendor build system requires MSVC on Windows. On MinGW or other non-MSVC
# toolchains, tgfx cannot be built; the framework falls back to a built-in
# OpenGL renderer (shader + VBO + FreeType for UTF-8 text).
#
# To enable tgfx: cmake -DNEOFLUX_USE_TGFX=ON ..
# (requires MSVC on Windows, or a pre-built tgfx package on other platforms)
# ---------------------------------------------------------------------------
option(NEOFLUX_USE_TGFX "Use tgfx as the rendering backend (requires MSVC on Windows)" OFF)

if(NEOFLUX_USE_TGFX)
  set(TGFX_USE_OPENGL ON CACHE BOOL "" FORCE)
  set(TGFX_USE_D3D12 OFF CACHE BOOL "" FORCE)
  set(TGFX_USE_METAL OFF CACHE BOOL "" FORCE)
  set(TGFX_USE_VULKAN OFF CACHE BOOL "" FORCE)
  set(TGFX_USE_WEBGPU OFF CACHE BOOL "" FORCE)
  set(TGFX_USE_FREETYPE ON CACHE BOOL "" FORCE)
  set(TGFX_BUILD_SVG OFF CACHE BOOL "" FORCE)
  set(TGFX_BUILD_PDF OFF CACHE BOOL "" FORCE)
  set(TGFX_BUILD_LAYERS OFF CACHE BOOL "" FORCE)
  set(TGFX_BUILD_HELLO2D OFF CACHE BOOL "" FORCE)
  add_subdirectory(tgfx)
endif()

message(STATUS "NeoFlux third-party dependencies configured")
