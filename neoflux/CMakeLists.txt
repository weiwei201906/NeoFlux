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
  src/renderers/tgfx_renderer.cpp
  src/widgets/media_widget.cpp
  src/media/mpv_media_player.cpp
  src/media/media_player_factory.cpp
)

# Platform-specific bridge implementations.
if(ANDROID OR (APPLE AND IOS))
  target_sources(neoflux PRIVATE src/renderers/mobile_bridge.cpp)
else()
  target_sources(neoflux PRIVATE src/renderers/glfw_bridge.cpp)
endif()

target_include_directories(neoflux
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_SOURCE_DIR}/thirdparty
)

# OpenGL is required for both the GLFW bridge and the OpenGL fallback renderer.
find_package(OpenGL REQUIRED)

# ---------------------------------------------------------------------------
# libmpv (optional desktop media backend)
#
# On Windows, cmake/DownloadMPV.cmake pulls the latest shinchiro/mpv-win32
# build (headers + libmpv-2.dll) into thirdparty/mpv-bundle on first configure.
# MSVC cannot link the MinGW import lib, so we regenerate mpv.lib from the DLL
# exports at configure time using dumpbin + the MSVC library manager.
# On Linux, pkg-config finds the system libmpv (apt install libmpv-dev).
# When mpv is present, NEOFLUX_HAS_MPV is defined.
# ---------------------------------------------------------------------------
include("${CMAKE_SOURCE_DIR}/cmake/DownloadMPV.cmake")

set(MPV_DEF "${MPV_BUNDLE_DIR}/mpv.def")

set(NEOFLUX_MPV_FOUND FALSE CACHE INTERNAL "libmpv availability")
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
  message(STATUS "libmpv media backend disabled")
endif()

target_link_libraries(neoflux
  PUBLIC
    taitank
    glog::glog
    gflags
    freetype
    OpenGL::GL
)

# tgfx is the required rendering backend: it performs all 2D rasterization,
# text layout, and GPU dispatch. GLFW remains only a window/context bridge.
if(TARGET tgfx)
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
target_compile_definitions(neoflux PUBLIC NEOFLUX_VERSION_STRING="${NEOFLUX_VERSION}")

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
