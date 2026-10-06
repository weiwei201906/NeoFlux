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
# tgfx GPU backend selection (compile-time, owned by tgfx).
#
# The GPU backend is tgfx's own concept. Pick exactly one of tgfx's native
# options at configure time, e.g.:
#   -DTGFX_USE_METAL=ON  (Apple; the platform default below)
#   -DTGFX_USE_D3D12=ON  (Windows)
#   -DTGFX_USE_VULKAN=ON (Windows/OHOS)
#   -DTGFX_USE_OPENGL=ON (Linux/Windows/Android; the desktop default)
# tgfx itself validates platform support and enforces single-backend builds;
# there is deliberately no runtime backend flag (tgfx permits exactly one GPU
# backend per build). Switching backends = reconfigure + rebuild.
#
# NeoFlux code contains NO backend branching of its own: the renderer only
# consumes tgfx's Device/Window/Surface abstractions. The mirrored TGFX_USE_*
# macro below exists because tgfx defines TGFX_USE_* PRIVATE (consumers cannot
# see them); the media module's GL interop and the renderer's surface
# acquisition need the same knowledge in NeoFlux sources, under tgfx's own
# naming.
#
# Platform defaults: Apple defaults to Metal (NeoFlux's Apple rendering goes
# through tgfx's MetalWindow; tgfx's OpenGL/CGL path would require an ObjC++
# app shell that does not exist yet). Everything else defaults to OpenGL,
# which covers Linux/Windows desktop (EGLWindow/WGLWindow) and Android
# (EGLWindow).
# ---------------------------------------------------------------------------
if(APPLE)
  set(TGFX_USE_METAL ON CACHE BOOL "tgfx Metal backend (Apple only)")
  set(TGFX_USE_OPENGL OFF CACHE BOOL "tgfx OpenGL/EGL backend (Linux/Win32/Android)")
  if(TGFX_USE_OPENGL)
    message(FATAL_ERROR
      "TGFX_USE_OPENGL is not supported on Apple: NeoFlux's Apple rendering "
      "path requires the tgfx Metal backend (-DTGFX_USE_METAL=ON, the "
      "default). tgfx's OpenGL/CGL window needs an ObjC++ app shell that "
      "NeoFlux does not provide.")
  endif()
else()
  set(TGFX_USE_OPENGL ON CACHE BOOL "tgfx OpenGL/EGL backend (Linux/Win32/Android)")
  set(TGFX_USE_METAL OFF CACHE BOOL "tgfx Metal backend (Apple only)")
endif()
set(TGFX_USE_D3D12 OFF CACHE BOOL "tgfx D3D12 backend (Windows only)")
set(TGFX_USE_VULKAN OFF CACHE BOOL "tgfx Vulkan backend")
set(TGFX_USE_WEBGPU OFF CACHE BOOL "tgfx WebGPU backend (web only)")

set(NEOFLUX_ACTIVE_BACKEND "")
if(TGFX_USE_VULKAN)
  add_compile_definitions(TGFX_USE_VULKAN=1)
  set(NEOFLUX_ACTIVE_BACKEND "vulkan")
elseif(TGFX_USE_D3D12)
  add_compile_definitions(TGFX_USE_D3D12=1)
  set(NEOFLUX_ACTIVE_BACKEND "d3d12")
elseif(TGFX_USE_METAL)
  add_compile_definitions(TGFX_USE_METAL=1)
  set(NEOFLUX_ACTIVE_BACKEND "metal")
elseif(TGFX_USE_OPENGL)
  add_compile_definitions(TGFX_USE_OPENGL=1)
  set(NEOFLUX_ACTIVE_BACKEND "opengl")
else()
  message(FATAL_ERROR
    "No tgfx GPU backend enabled: turn on exactly one of TGFX_USE_OPENGL / "
    "TGFX_USE_VULKAN / TGFX_USE_D3D12 / TGFX_USE_METAL.")
endif()

set(TGFX_USE_FREETYPE ON CACHE BOOL "" FORCE)
set(TGFX_BUILD_SVG OFF CACHE BOOL "" FORCE)
set(TGFX_BUILD_PDF OFF CACHE BOOL "" FORCE)
set(TGFX_BUILD_LAYERS OFF CACHE BOOL "" FORCE)
set(TGFX_BUILD_HELLO2D OFF CACHE BOOL "" FORCE)

# TGFX's CMake unconditionally checks for shaderc/third_party/glslang and runs
# git-sync-deps when absent. OpenGL does not need shaderc, and Metal gets its
# compiler headers/libraries from the platform package manager below.
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tgfx/third_party/shaderc/third_party/glslang")
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tgfx/third_party/shaderc/third_party/glslang")
endif()
# tgfx includes third_party/vendor_tools/vendor.cmake which defines
# add_vendor_target / find_vendor_libraries / merge_libraries_into. On CI the
# vendor_tools directory is not synced. The OpenGL backend does not use tgfx's
# vendored third-party targets (we build freetype ourselves), so we write a
# no-op stub defining all three functions as empty.
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tgfx/third_party/vendor_tools/vendor.cmake")
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tgfx/third_party/vendor_tools")
  file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/tgfx/third_party/vendor_tools/vendor.cmake"
"# No-op vendor.cmake stub for OpenGL-only CI builds.
function(add_vendor_target targetName)
  # Create an empty interface library so add_dependencies(tgfx tgfx-vendor) works.
  if(NOT TARGET \${targetName})
    add_library(\${targetName} INTERFACE)
  endif()
endfunction()
function(find_vendor_libraries target)
  set(\${ARGV2} \"\" PARENT_SCOPE)
  set(\${ARGV4} \"\" PARENT_SCOPE)
endfunction()
function(merge_libraries_into target)
endfunction()
")
endif()
add_subdirectory(tgfx)

if(APPLE AND TGFX_USE_METAL)
  find_path(SHADERC_INCLUDE_DIR shaderc/shaderc.hpp REQUIRED)
  find_library(SHADERC_LIBRARY NAMES shaderc_shared REQUIRED)
  find_path(SPIRV_CROSS_INCLUDE_DIR spirv_cross/spirv_msl.hpp REQUIRED)
  find_library(SPIRV_CROSS_CORE_LIBRARY NAMES spirv-cross-core REQUIRED)
  find_library(SPIRV_CROSS_GLSL_LIBRARY NAMES spirv-cross-glsl REQUIRED)
  find_library(SPIRV_CROSS_MSL_LIBRARY NAMES spirv-cross-msl REQUIRED)
  target_include_directories(tgfx SYSTEM PRIVATE
    ${SHADERC_INCLUDE_DIR}
    ${SPIRV_CROSS_INCLUDE_DIR}
  )
  target_link_libraries(tgfx
    ${SHADERC_LIBRARY}
    ${SPIRV_CROSS_MSL_LIBRARY}
    ${SPIRV_CROSS_GLSL_LIBRARY}
    ${SPIRV_CROSS_CORE_LIBRARY}
  )
endif()

# ---------------------------------------------------------------------------
# Vendored tgfx third-party deps that tgfx does NOT git-track.
#
# tgfx normally populates thirdparty/tgfx/third_party/{concurrentqueue,skcms}
# (and compiles them) via gclient + depsync at configure time. On a fresh CI
# clone (no depsync, offline) those directories are empty, so tgfx fails with
#   fatal error: concurrentqueue.h: No such file or directory
#   fatal error: skcms.h: No such file or directory
#
# We control tiny copies under thirdparty/ here:
#   - concurrentqueue is header-only (cameron314).
#   - skcms is a small self-contained color-management lib; we build it from
#     source into a static archive and link it into tgfx so the tgfx-vendor
#     link resolves (tgfx unconditionally calls skcms_* from ColorSpace.cpp,
#     ColorSpaceXformSteps.cpp, FTScalerContext.cpp, CopyPixels.cpp, ...).
#
# tgfx already adds its own (empty) third_party/{skcms,concurrentqueue} dirs as
# PRIVATE include paths; appending our real dirs makes the missing headers
# resolve without touching the tgfx submodule tree.
# ---------------------------------------------------------------------------
set(NEOFLUX_VENDORED_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# concurrentqueue: header-only.
add_library(neoflux_concurrentqueue INTERFACE)
target_include_directories(neoflux_concurrentqueue INTERFACE
    ${NEOFLUX_VENDORED_DIR}/concurrentqueue)

# skcms: compile the implementation + the BASELINE CPU-dispatch unit only.
# The Hsw (Haswell/AVX2+F16C) and Skx (Skylake-X/AVX-512) units use intrinsics
# that require per-TU -mavx2/-mf16c/-mavx512 flags, which differ across GCC/Clang/
# MSVC and CI runners. We build the portable baseline (scalar + SSE2, on by
# default for x86-64) and define SKCMS_DISABLE_{HSW,SKX} so skcms.cc's runtime
# dispatch never references the hsw/skx symbols.
add_library(neoflux_skcms STATIC
    ${NEOFLUX_VENDORED_DIR}/skcms/skcms.cc
    ${NEOFLUX_VENDORED_DIR}/skcms/src/skcms_TransformBaseline.cc)
target_compile_definitions(neoflux_skcms PRIVATE
    SKCMS_DISABLE_HSW SKCMS_DISABLE_SKX)
target_include_directories(neoflux_skcms PUBLIC
    ${NEOFLUX_VENDORED_DIR}/skcms)
set_target_properties(neoflux_skcms PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

# Google Highway (hwy): tgfx's SIMD core (BoxFilterDownsampleSIMD, MatrixSIMD,
# RectSIMD, VecSIMD, TileSortCompareFuncSIMD) unconditionally includes
# hwy/foreach_target.h and hwy/highway.h. tgfx normally gets hwy as a depsync
# prebuilt; offline CI has nothing. Vendor the hwy/ headers and compile the core
# dispatch runtime (CPU detection + per-target registration). We deliberately
# do NOT glob hwy/tests|hwy/examples.
add_library(neoflux_hwy STATIC
    ${NEOFLUX_VENDORED_DIR}/highway/hwy/targets.cc
    ${NEOFLUX_VENDORED_DIR}/highway/hwy/per_target.cc
    ${NEOFLUX_VENDORED_DIR}/highway/hwy/abort.cc
    ${NEOFLUX_VENDORED_DIR}/highway/hwy/timer.cc
    ${NEOFLUX_VENDORED_DIR}/highway/hwy/print.cc
    ${NEOFLUX_VENDORED_DIR}/highway/hwy/aligned_allocator.cc)
target_include_directories(neoflux_hwy PUBLIC
    ${NEOFLUX_VENDORED_DIR}/highway)
set_target_properties(neoflux_hwy PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

# pathkit: tgfx's PathRef.h / PathTriangulator.cpp unconditionally include
# "pathkit.h" and call pk::SkPath::toTriangles / toAATriangles. pathkit is a
# self-contained slice of Skia pathops (no standalone upstream repo; normally a
# depsync prebuilt). Vendor it and compile src/**/*.cpp into a static lib.
file(GLOB_RECURSE NEOFLUX_PATHKIT_SOURCES
    ${NEOFLUX_VENDORED_DIR}/pathkit/src/*.cpp)
add_library(neoflux_pathkit STATIC ${NEOFLUX_PATHKIT_SOURCES})
target_include_directories(neoflux_pathkit PUBLIC
    ${NEOFLUX_VENDORED_DIR}/pathkit)
# SkNx_sse.h is textually included from inside `namespace pk`; on MinGW GCC
# that pulls the *intrin.h headers into namespace pk (declaring pk::__m128 etc.)
# and fails to compile. Force pathkit's portable scalar (non-SIMD) SkNx path.
target_compile_definitions(neoflux_pathkit PRIVATE SKNX_NO_SIMD)
set_target_properties(neoflux_pathkit PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

# Make the already-created tgfx target find our headers and link the vendored
# static libs.
# NOTE: tgfx/CMakeLists.txt already called target_link_libraries(tgfx ...)
# with the PLAIN signature (line ~777), so we must use the plain form here
# too; CMake forbids mixing plain and keyword signatures on one target.
target_include_directories(tgfx PRIVATE
    ${NEOFLUX_VENDORED_DIR}/concurrentqueue
    ${NEOFLUX_VENDORED_DIR}/skcms
    ${NEOFLUX_VENDORED_DIR}/highway
    ${NEOFLUX_VENDORED_DIR}/pathkit)
target_link_libraries(tgfx neoflux_concurrentqueue neoflux_skcms neoflux_hwy neoflux_pathkit freetype)

# tgfx's Windows GL backend (WGLDevice/WGLGPU) calls wgl* entry points that
# live in opengl32.dll; link it on Windows so the final app/test binaries link.
if (WIN32)
    target_link_libraries(tgfx opengl32 gdi32)
endif ()

# MinGW GCC 14 is stricter than Linux GCC 11 on tgfx's FTPath.cpp freetype
# outline casts (unsigned char*/unsigned short* vs freetype's char*/short*):
#   error: invalid conversion from 'unsigned char*' to 'char*' [-fpermissive]
# The diagnostic itself suggests -fpermissive; accept it (these become
# warnings, not -Werror trips).
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(tgfx PRIVATE -fpermissive)
endif ()

# tgfx's public headers are not -Werror-clean (old-style casts in
# ColorSpace.h, shadowing params in Point.h/Rect.h). neoflux compiles with
# -Werror and includes those headers transitively. Expose tgfx's include dir
# to consumers as SYSTEM so warnings in vendored headers are suppressed.
target_include_directories(tgfx SYSTEM INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/tgfx/include)

# MinGW's dwrite_3.h lacks the DWrite 3 GetMatchingFonts(family,weight,
# stretch,style) overload that tgfx's SystemFont.cpp uses, so that TU cannot
# compile with MinGW GCC. Drop it and link a tiny no-op stub for the two
# SystemFont::MakeFromName entry points (system-font lookup is not needed to
# build on CI). Real MSVC Windows SDK keeps the original file.
if (MINGW)
    get_target_property(_tgfx_srcs_mingw tgfx SOURCES)
    if (_tgfx_srcs_mingw)
        list(FILTER _tgfx_srcs_mingw EXCLUDE REGEX "SystemFont\\.cpp$")
        set_target_properties(tgfx PROPERTIES SOURCES "${_tgfx_srcs_mingw}")
    endif ()
    target_sources(tgfx PRIVATE ${NEOFLUX_VENDORED_DIR}/stubs/SystemFont_mingw.cpp)
endif ()

# Strip optional raster image codecs. On desktop tgfx turns PNG/JPEG/WebP
# decode+encode ON by default (and freetype force-enables PNG_DECODE), and
# expects libpng/zlib/libjpeg-turbo/libwebp as depsync prebuilts. A fresh CI
# clone has none of them, and the GL backend does not need image file I/O to
# build. Drop the codec sources and remove their compile macros from the
# already-created tgfx target; ImageCodec.cpp's #ifdef dispatch then compiles
# to no-op paths (decoding unsupported at runtime, which is fine for CI build).
# Only pdf/DeflateStream.cpp uses zlib directly and it is built only when
# TGFX_BUILD_PDF is on (off here).
get_target_property(_tgfx_defs tgfx COMPILE_DEFINITIONS)
if (_tgfx_defs)
    list(FILTER _tgfx_defs EXCLUDE REGEX "TGFX_USE_(PNG|JPEG|WEBP)")
    set_target_properties(tgfx PROPERTIES COMPILE_DEFINITIONS "${_tgfx_defs}")
endif ()
get_target_property(_tgfx_srcs tgfx SOURCES)
if (_tgfx_srcs)
    list(FILTER _tgfx_srcs EXCLUDE REGEX "codecs[\\/](png|jpeg|webp)[\\/]")
    set_target_properties(tgfx PROPERTIES SOURCES "${_tgfx_srcs}")
endif ()

message(STATUS "NeoFlux third-party dependencies configured")
