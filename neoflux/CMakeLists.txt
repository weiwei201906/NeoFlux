# =============================================================================
# NeoFlux framework library
#
# This directory builds the `neoflux` STATIC library: the full framework body
# (application layer, core utilities, render backends, widgets, media). It is a
# pure library target. User quick-start code lives at the top-level src/ and
# links against this target.
#
# Public include root: neoflux/include  ->  #include <neoflux/widgets/widget.h>
# =============================================================================

# ---------------------------------------------------------------------------
# Core library
# ---------------------------------------------------------------------------
add_library(neoflux STATIC
  src/core/flags.cpp
  src/core/ring_queue.cpp
  src/core/font_manager.cpp
  src/core/types.cpp
  src/core/animation.cpp
  src/core/version.cpp
  src/widgets/widget.cpp
  src/widgets/container.cpp
  src/widgets/text.cpp
  src/widgets/button.cpp
  src/widgets/scroll_view.cpp
  src/widgets/sized_box.cpp
  src/widgets/expanded.cpp
  src/widgets/draggable.cpp
  src/widgets/divider.cpp
  src/widgets/padding.cpp
  src/widgets/center.cpp
  src/widgets/align.cpp
  src/widgets/card.cpp
  src/widgets/progress_indicator.cpp
  src/widgets/switch.cpp
  src/widgets/checkbox.cpp
  src/widgets/route_registry.cpp
  src/apps/application.cpp
  src/apps/event_loop.cpp
  src/renderers/render_layer.cpp
  src/renderers/render_command.cpp
  src/renderers/render_context.cpp
  src/renderers/frame_image_registry.cpp
  src/renderers/tgfx_renderer.cpp
  src/widgets/media_widget.cpp
  src/media/mpv_media_player.cpp
  src/media/media_player_factory.cpp
)

# Platform-specific bridge implementations.
#
# This predicate MUST stay identical to the one deciding NEOFLUX_PLATFORM_MOBILE
# vs NEOFLUX_PLATFORM_DESKTOP below. The two answer different questions -- which
# bridge .cpp gets compiled, and which of them is allowed to be non-empty -- but
# they must never disagree: RenderLayer::Start() calls CreateMobileBridge() for
# every non-desktop build, so compiling glfw_bridge.cpp while defining
# NEOFLUX_PLATFORM_DESKTOP is the only other consistent combination. Mixing them
# yields an empty mobile bridge translation unit and an undefined symbol at link
# time. APPLE is deliberately not tested: whether it is set for an iOS
# toolchain depends on CMP0025, whereas IOS is set by every iOS toolchain.
if(ANDROID OR IOS)
  target_sources(neoflux PRIVATE src/renderers/mobile_bridge.cpp)
else()
  target_sources(neoflux PRIVATE src/renderers/glfw_bridge.cpp)
endif()

# ---------------------------------------------------------------------------
# Platform-native tuning layer (src/native/)
#
# Low-level per-platform tuning ONLY: thread scheduling/priority, timer
# resolution, CPU feature detection (see src/native/README.md for the scope
# contract). No rendering, no GL, no backend code -- GPU backends live behind
# tgfx (the GPU backend is tgfx's own TGFX_USE_* compile-time choice; see thirdparty/CMakeLists.txt).
# ---------------------------------------------------------------------------
# Hand-written assembly primitives (src/native/asm/)
#
# Project policy: no inline asm in C++ sources, and no CPU intrinsics outside
# this dispatch layer. Every instruction the framework needs that portable C++
# cannot express is one entry in asm_symbols.h, implemented once per
# architecture:
#
#   cpuid_x86.S        CPUID leaf/subleaf reads         x86, non-MSVC
#   xgetbv.S           XCR0 read                        x86, non-MSVC
#   prefetch_x86.S     PREFETCHT0 read/write hints      x86-64, non-MSVC
#   prefetch_aarch64.S PRFM read/write hints            AArch64
#   premultiply_*.S    SSE2 / NEON premultiply kernels  x86-64 / AArch64
#
# Sharing one primitive instead of one intrinsic per compiler is what stops a
# change verified on Linux from breaking Windows: the platform code above this
# layer calls a plain C function with the same signature everywhere, and only
# the argument-register mapping differs, hidden inside abi_macros.inc.
#
# MSVC is excluded because it assembles MASM, not GAS. An MSVC build therefore
# takes the routes in cpuid_bits.h (__cpuidex) and reports avx2 = false rather
# than guessing, and the SIMD kernels fall back to the portable scalar path.
#
# Sourcing a .S and defining its macro are two halves of one decision: the macro
# is what C++ tests, so a build that added the source without the macro would
# silently keep the scalar path, and one that defined the macro without the
# source would fail to link. Both come from the same predicate on purpose.
# ---------------------------------------------------------------------------
enable_language(ASM)

set(NEOFLUX_NATIVE_ASM_SRC "")
set(NEOFLUX_ASM_SUMMARY "")

if(NOT MSVC)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64|AMD64|i[3-6]86)")
    list(APPEND NEOFLUX_NATIVE_ASM_SRC
      src/native/asm/xgetbv.S
      src/native/asm/cpuid_x86.S
      src/native/asm/prefetch_x86.S)
    target_compile_definitions(neoflux PRIVATE
      NEOFLUX_NATIVE_ASM_XGETBV=1
      NEOFLUX_NATIVE_ASM_CPUID=1
      NEOFLUX_NATIVE_ASM_PREFETCH=1)
    set(NEOFLUX_ASM_SUMMARY "xgetbv, cpuid, prefetch (x86)")
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(aarch64|arm64|ARM64)")
    list(APPEND NEOFLUX_NATIVE_ASM_SRC src/native/asm/prefetch_aarch64.S)
    target_compile_definitions(neoflux PRIVATE NEOFLUX_NATIVE_ASM_PREFETCH=1)
    set(NEOFLUX_ASM_SUMMARY "prefetch (AArch64)")
  endif()
endif()

if(WIN32)
  target_sources(neoflux PRIVATE
    src/native/windows/platform_win32.cpp
    ${NEOFLUX_NATIVE_ASM_SRC})
  # winmm: timeBeginPeriod (frame pacing timer resolution)
  # avrt:  MMCSS render-thread registration (AvSetMmThreadCharacteristicsW)
  target_link_libraries(neoflux PUBLIC winmm avrt)
elseif(APPLE)
  target_sources(neoflux PRIVATE
    src/native/apple/platform_apple.cpp
    src/native/apple/cache_topology_apple.cpp
    ${NEOFLUX_NATIVE_ASM_SRC})
elseif(ANDROID OR UNIX)
  target_sources(neoflux PRIVATE
    src/native/linux/platform_linux.cpp
    ${NEOFLUX_NATIVE_ASM_SRC})
else()
  target_sources(neoflux PRIVATE src/native/platform_common.cpp)
endif()

if(NEOFLUX_ASM_SUMMARY)
  message(STATUS "Native assembly primitives: ${NEOFLUX_ASM_SUMMARY}")
elseif(MSVC)
  message(STATUS "Native assembly primitives: none (MSVC uses cpuid_bits.h "
                 "intrinsics; SIMD kernels fall back to the scalar path)")
else()
  message(STATUS "Native assembly primitives: none for "
                 "${CMAKE_SYSTEM_PROCESSOR}")
endif()

# ---------------------------------------------------------------------------
# SIMD kernels (src/native/asm/premultiply_rgba_*.S)
#
# Selected by TARGET ARCHITECTURE, not by OS: SSE2 is an architectural
# baseline of x86-64 and Advanced SIMD (NEON) is one of AArch64, so neither
# needs a runtime CPU feature probe (unlike xgetbv.S, which exists precisely
# because AVX2 is NOT a baseline). See src/native/simd_kernels.h.
#
# MSVC is excluded: it assembles MASM (.asm), not GAS (.S). Shipping a MASM
# translation we cannot verify here would be worse than falling back, and the
# portable scalar kernel in simd_kernels.cpp is always compiled in, so the
# build simply degrades to kScalar.
#
# The macro is PRIVATE because native/asm/asm_symbols.h and native/
# simd_kernels.h are both internal headers (src/ is a PRIVATE include dir);
# nothing outside the neoflux target can form a reference to the C ABI symbol
# anyway.
# ---------------------------------------------------------------------------
if(NOT MSVC)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64|AMD64)")
    target_sources(neoflux PRIVATE
      src/native/asm/premultiply_rgba_x86_64.S)
    target_compile_definitions(neoflux PRIVATE
      NEOFLUX_NATIVE_ASM_PREMULTIPLY_X86_64=1)
    message(STATUS "Native SIMD kernel: SSE2 premultiply (x86-64 assembly)")
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(aarch64|arm64|ARM64)")
    target_sources(neoflux PRIVATE
      src/native/asm/premultiply_rgba_aarch64.S)
    target_compile_definitions(neoflux PRIVATE
      NEOFLUX_NATIVE_ASM_PREMULTIPLY_AARCH64=1)
    message(STATUS "Native SIMD kernel: NEON premultiply (AArch64 assembly)")
  else()
    message(STATUS "Native SIMD kernel: none for "
                   "${CMAKE_SYSTEM_PROCESSOR}; using the portable scalar path")
  endif()
else()
  message(STATUS "MSVC toolchain: native SIMD kernel disabled "
                 "(MASM source not shipped), using the portable scalar path")
endif()

# The dispatch layer is ALWAYS compiled in: it is also what finishes the tail
# the kernel leaves behind, and the whole implementation when no assembly was
# linked.
target_sources(neoflux PRIVATE src/native/simd_kernels.cpp)

target_include_directories(neoflux
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_SOURCE_DIR}/thirdparty
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ---------------------------------------------------------------------------
# GPU backend link dependency
#
# All drawing goes through tgfx, which creates and owns the GPU context and
# surface for the platform window (EGL/WGL on the OpenGL backend, Vulkan/
# D3D12/Metal devices elsewhere). NeoFlux contains no GL calls of its own, so
# there is deliberately no find_package(OpenGL) here: tgfx links the windowing
# system's GL/EGL libraries itself (see tgfx's CMakeLists.txt).
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# libmpv (optional desktop media backend) -- BACKEND-INDEPENDENT
#
# Media playback is no longer tied to the tgfx backend: libmpv renders frames
# into a CPU buffer through its software render API (MPV_RENDER_API_TYPE_SW),
# the frame is published as a tgfx image, and TgfxRenderer composites it through
# the backend-agnostic kDrawTexture path (Canvas::drawImageRect). Nothing in
# that path is OpenGL-specific, so the media backend is gated ONLY on libmpv
# being available. When it is not, NEOFLUX_HAS_MPV stays undefined,
# MpvMediaPlayer compiles as a no-op stub and MediaWidget renders its
# placeholder -- on every tgfx backend (this is the CI configuration).
#
# On Windows, cmake/DownloadMPV.cmake pulls the latest shinchiro/mpv-win32
# build (headers + libmpv-2.dll) into thirdparty/mpv-bundle on first configure.
# MSVC cannot link the MinGW import lib, so we regenerate mpv.lib from the DLL
# exports at configure time using dumpbin + the MSVC library manager.
# On Linux, pkg-config finds the system libmpv (apt install libmpv-dev).
# When mpv is present, NEOFLUX_HAS_MPV is defined.
# ---------------------------------------------------------------------------
set(NEOFLUX_MPV_FOUND FALSE CACHE INTERNAL "libmpv availability")
include("${CMAKE_SOURCE_DIR}/cmake/DownloadMPV.cmake")

set(MPV_DEF "${MPV_BUNDLE_DIR}/mpv.def")

if(WIN32)
  if(EXISTS "${MPV_BUNDLE_DIR}/include/mpv/client.h" AND EXISTS "${MPV_DLL}")
    if(NOT EXISTS "${MPV_IMPORTED_LIB}")
      find_program(DUMPBIN_EXECUTABLE NAMES dumpbin)
      find_program(LIB_EXECUTABLE NAMES lib)
      find_package(Python3 COMPONENTS Interpreter)
      if(DUMPBIN_EXECUTABLE AND LIB_EXECUTABLE AND Python3_Interpreter_FOUND)
        message(STATUS "Generating MSVC import library for libmpv: ${MPV_IMPORTED_LIB}")
        execute_process(
          COMMAND "${Python3_EXECUTABLE}"
                  "${CMAKE_SOURCE_DIR}/cmake/gen_mpv_def.py"
                  "${DUMPBIN_EXECUTABLE}" "${MPV_DLL}" "${MPV_DEF}"
          RESULT_VARIABLE MPV_GEN_DEF_RESULT
          ERROR_VARIABLE MPV_GEN_DEF_ERR)
        if(MPV_GEN_DEF_RESULT EQUAL 0)
          execute_process(
            COMMAND "${LIB_EXECUTABLE}" /nologo
                    /def:${MPV_DEF} /machine:x64 /out:${MPV_IMPORTED_LIB}
            RESULT_VARIABLE MPV_GEN_LIB_RESULT
            ERROR_VARIABLE MPV_GEN_LIB_ERR)
        endif()
        if(MPV_GEN_DEF_RESULT EQUAL 0 AND MPV_GEN_LIB_RESULT EQUAL 0)
          set(NEOFLUX_MPV_FOUND TRUE CACHE INTERNAL "libmpv availability" FORCE)
        else()
          message(WARNING "libmpv import-lib generation failed: "
                          "${MPV_GEN_DEF_RESULT} ${MPV_GEN_LIB_RESULT} "
                          "${MPV_GEN_DEF_ERR} ${MPV_GEN_LIB_ERR}")
        endif()
      else()
        message(STATUS "libmpv: dumpbin/lib/python not found; media backend disabled")
      endif()
    else()
      set(NEOFLUX_MPV_FOUND TRUE CACHE INTERNAL "libmpv availability" FORCE)
    endif()
  else()
    message(STATUS "libmpv bundle not found under ${MPV_BUNDLE_DIR}; media backend disabled")
  endif()
else()
  if(MPV_FOUND)
    set(NEOFLUX_MPV_FOUND TRUE CACHE INTERNAL "libmpv availability" FORCE)
  endif()
endif()

if(NEOFLUX_MPV_FOUND)
  message(STATUS "libmpv media backend ENABLED (${MPV_IMPORTED_LIB})")
else()
  message(STATUS "libmpv media backend disabled (media path is backend-agnostic)")
endif()

target_link_libraries(neoflux
  PUBLIC
    taitank
    glog::glog
    gflags
    freetype
)

# The system OpenGL library is not linked at all: NeoFlux has no GL calls,
# and tgfx brings its own EGL/WGL/GLES linkage (see the backend comment above).

# tgfx is the REQUIRED rendering backend: it performs all 2D rasterization,
# text layout, GPU dispatch, and owns the platform's graphics surface. There
# is no build without it. Initialize the thirdparty/tgfx submodule
# (git submodule update --init --recursive) before configuring.
if(NOT TARGET tgfx)
  message(FATAL_ERROR
    "tgfx was not found as a CMake target. NeoFlux requires tgfx to build: "
    "make sure the thirdparty/tgfx submodule is initialized "
    "(git submodule update --init --recursive) and add_subdirectory(tgfx) "
    "runs in thirdparty/CMakeLists.txt.")
endif()
target_link_libraries(neoflux PRIVATE tgfx)
target_include_directories(neoflux PRIVATE
  ${tgfx_SOURCE_DIR}/src
  ${tgfx_SOURCE_DIR}/include
)

# GLFW is only available on desktop platforms.
if(TARGET glfw)
  target_link_libraries(neoflux PRIVATE glfw)
endif()

# Link the libmpv import library and expose the media backend. NEOFLUX_HAS_MPV
# is also propagated to test executables that link neoflux so they can exercise
# the MpvMediaPlayer implementation.
if(NEOFLUX_MPV_FOUND)
  if(WIN32)
    target_include_directories(neoflux PRIVATE "${MPV_BUNDLE_DIR}/include")
    target_link_libraries(neoflux PRIVATE "${MPV_IMPORTED_LIB}")
  else()
    target_include_directories(neoflux PRIVATE ${MPV_INCLUDE_DIRS})
    target_link_libraries(neoflux PRIVATE ${MPV_LIBRARIES})
  endif()
  target_compile_definitions(neoflux PRIVATE NEOFLUX_HAS_MPV=1)
  target_compile_definitions(neoflux INTERFACE NEOFLUX_HAS_MPV=1)
endif()

target_compile_features(neoflux PUBLIC cxx_std_20)

# Bake the (git-derived) project version into the binary as a string literal,
# exposed via neoflux::Version() in neoflux/version.h. Use the full tag string
# (including any prerelease suffix such as "-alpha"); PROJECT_VERSION holds only
# the numeric components CMake accepts.
#
# The replacement list is parenthesised. `NEOFLUX_VERSION_STRING="0.3.1"` is a
# perfectly valid definition, but an unparenthesised macro body is exactly what
# clang-tidy's bugprone-macro-parentheses reports, and this definition is
# visible to every translation unit that includes neoflux/version.h. The extra
# parentheses are invisible at the use site (`return NEOFLUX_VERSION_STRING;`).
target_compile_definitions(neoflux PUBLIC "NEOFLUX_VERSION_STRING=\"${NEOFLUX_VERSION}\"")

# Strict warnings for neoflux code only (not third-party deps). Conversion
# warnings are intentionally fatal so integer widths and signedness are explicit
# in framework code; third-party headers remain outside this policy.
# -Wno-error=attributes: glog headers emit dllimport attribute warnings on
# Windows; warning is still shown, just not fatal.
target_compile_options(neoflux PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror -Wno-error=attributes>
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
# Tests (framework gtest). enable_testing() is called at the top-level when
# NEOFLUX_BUILD_TESTS is on so `ctest` works from the build root.
# ---------------------------------------------------------------------------
if(NEOFLUX_BUILD_TESTS)
  add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# Install rules
# ---------------------------------------------------------------------------
install(TARGETS neoflux
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/neoflux DESTINATION include)
