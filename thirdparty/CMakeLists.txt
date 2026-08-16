# =============================================================================
# Third-party dependencies for NeoFlux
#
# All dependencies are fetched via CMake FetchContent and stored under
# thirdparty/_deps to keep the source tree self-contained.
# =============================================================================

include(FetchContent)

# Store fetched content under thirdparty to satisfy project layout requirement
set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/_deps"
    CACHE PATH "FetchContent base directory" FORCE)
set(FETCHCONTENT_QUIET OFF)

# ---------------------------------------------------------------------------
# Google glog - logging library
# ---------------------------------------------------------------------------
FetchContent_Declare(
  glog
  GIT_REPOSITORY https://github.com/google/glog.git
  GIT_TAG        v0.7.1
  GIT_SHALLOW    TRUE
)
set(WITH_GFLAGS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glog)

# ---------------------------------------------------------------------------
# Google gflags - command-line flag parsing
# ---------------------------------------------------------------------------
FetchContent_Declare(
  gflags
  GIT_REPOSITORY https://github.com/gflags/gflags.git
  GIT_TAG        v2.2.2
  GIT_SHALLOW    TRUE
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(gflags)

# ---------------------------------------------------------------------------
# Google Test - unit testing framework
# ---------------------------------------------------------------------------
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
  GIT_SHALLOW    TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ---------------------------------------------------------------------------
# GLFW - desktop window / input management
# ---------------------------------------------------------------------------
if(NOT ANDROID AND NOT IOS)
  FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
  )
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(glfw)
endif()

# ---------------------------------------------------------------------------
# Taitank - Tencent cross-platform layout engine (flexbox)
# ---------------------------------------------------------------------------
FetchContent_Declare(
  taitank
  GIT_REPOSITORY https://github.com/Tencent/taitank.git
  GIT_TAG        main
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(taitank)

# ---------------------------------------------------------------------------
# tgfx - Tencent lightweight 2D graphics rendering library
#
# tgfx uses `depsync` (npm package) to manage vendor dependencies, and its
# vendor build system requires MSVC on Windows. On MinGW or other non-MSVC
# toolchains, tgfx cannot be built; the framework falls back to a built-in
# OpenGL renderer (shader + VBO + stb_truetype for UTF-8 text).
#
# To enable tgfx: cmake -DNEOFLUX_USE_TGFX=ON .. (requires MSVC on Windows,
# or a pre-built tgfx package on other platforms).
# ---------------------------------------------------------------------------
option(NEOFLUX_USE_TGFX "Use tgfx as the rendering backend (requires MSVC on Windows)" OFF)

if(NEOFLUX_USE_TGFX)
  FetchContent_Declare(
    tgfx
    GIT_REPOSITORY https://github.com/Tencent/tgfx.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
  )
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
  FetchContent_MakeAvailable(tgfx)
endif()

message(STATUS "NeoFlux third-party dependencies configured")
