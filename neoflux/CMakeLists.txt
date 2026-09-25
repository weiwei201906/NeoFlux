# =============================================================================
# NeoFlux - framework library (self-contained build unit)
#
# Adds the `neoflux` static library target. The host project is expected to
# provide the third-party targets (glog, gflags, glfw, taitank, freetype,
# OpenGL) -- e.g. by add_subdirectory(thirdparty) before this file, or by
# find_package. See README.md for the canonical host setup.
#
# To consume NeoFlux as a git submodule inside your own project:
#   add_subdirectory(thirdparty/neoflux)   # or your submodule path
# and link against the `neoflux` target.
# =============================================================================

# Version of the framework (independent of the host project()).
set(NEOFLUX_VERSION_MAJOR 0)
set(NEOFLUX_VERSION_MINOR 1)
set(NEOFLUX_VERSION_PATCH 0)

# ---------------------------------------------------------------------------
# Options (cache variables so the host can override them)
# ---------------------------------------------------------------------------
option(NEOFLUX_BUILD_TESTS "Build NeoFlux unit tests" OFF)
option(NEOFLUX_ENABLE_CLANG_TIDY "Enable clang-tidy as a build step" OFF)
option(NEOFLUX_USE_TGFX "Use tgfx rendering backend (requires MSVC on Windows)" OFF)

# ---------------------------------------------------------------------------
# Compiler flags (shared by framework and host)
# ---------------------------------------------------------------------------
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerFlags.cmake)

# ---------------------------------------------------------------------------
# clang-tidy integration
# ---------------------------------------------------------------------------
if(NEOFLUX_ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy clang-tidy-18 clang-tidy-17 clang-tidy-16)
  if(CLANG_TIDY_EXE)
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
    set(CMAKE_CXX_CLANG_TIDY
      "${CLANG_TIDY_EXE}"
      "-config-file=${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy"
    )
  else()
    message(STATUS "clang-tidy not found - skipping static analysis")
  endif()
endif()

# ---------------------------------------------------------------------------
# Framework sources (all implementations live under neoflux/src)
# ---------------------------------------------------------------------------
set(NEOFLUX_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_library(neoflux STATIC
  ${NEOFLUX_SRC_DIR}/core/ring_queue.cpp
  ${NEOFLUX_SRC_DIR}/core/font_manager.cpp
  ${NEOFLUX_SRC_DIR}/core/types.cpp
  ${NEOFLUX_SRC_DIR}/core/object_pool.cpp
  ${NEOFLUX_SRC_DIR}/core/mapped_memory.cpp
  ${NEOFLUX_SRC_DIR}/widget/widget.cpp
  ${NEOFLUX_SRC_DIR}/widget/widget_arena.cpp
  ${NEOFLUX_SRC_DIR}/widget/container.cpp
  ${NEOFLUX_SRC_DIR}/widget/text.cpp
  ${NEOFLUX_SRC_DIR}/widget/button.cpp
  ${NEOFLUX_SRC_DIR}/widget/scroll_view.cpp
  ${NEOFLUX_SRC_DIR}/widget/sized_box.cpp
  ${NEOFLUX_SRC_DIR}/widget/expanded.cpp
  ${NEOFLUX_SRC_DIR}/widget/draggable.cpp
  ${NEOFLUX_SRC_DIR}/widget/text_field.cpp
  ${NEOFLUX_SRC_DIR}/widget/media_widget.cpp
  ${NEOFLUX_SRC_DIR}/widget/route_registry.cpp
  ${NEOFLUX_SRC_DIR}/app/application.cpp
  ${NEOFLUX_SRC_DIR}/app/event_loop.cpp
  ${NEOFLUX_SRC_DIR}/render/render_layer.cpp
  ${NEOFLUX_SRC_DIR}/render/render_command.cpp
  ${NEOFLUX_SRC_DIR}/render/render_context.cpp
  ${NEOFLUX_SRC_DIR}/render/tgfx_renderer.cpp
  ${NEOFLUX_SRC_DIR}/native/gl_renderer.cpp
  ${NEOFLUX_SRC_DIR}/native/desktop/mpv_media_player.cpp
  ${NEOFLUX_SRC_DIR}/media/media_player_factory.cpp
)

# Platform-specific bridge implementations.
if(ANDROID OR (APPLE AND IOS))
  target_sources(neoflux PRIVATE ${NEOFLUX_SRC_DIR}/native/mobile/mobile_bridge.cpp)
else()
  target_sources(neoflux PRIVATE ${NEOFLUX_SRC_DIR}/native/desktop/glfw_bridge.cpp)
endif()

# Platform-specific media player implementations.
if(ANDROID)
  target_sources(neoflux PRIVATE ${NEOFLUX_SRC_DIR}/native/android/android_media_player.cpp)
elseif(APPLE AND IOS)
  target_sources(neoflux PRIVATE ${NEOFLUX_SRC_DIR}/native/ios/ios_media_player.mm)
  set_source_files_properties(${NEOFLUX_SRC_DIR}/native/ios/ios_media_player.mm
    PROPERTIES COMPILE_FLAGS "-x objective-c++")
endif()

# Include platform-specific build configuration (Android NDK / iOS frameworks).
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/android.cmake OPTIONAL)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ios.cmake OPTIONAL)

target_include_directories(neoflux
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty
)

# OpenGL is required for both the GLFW bridge and the OpenGL fallback renderer.
find_package(OpenGL REQUIRED)

# Third-party targets are provided by the host project. Fail early with a
# clear message when they are missing so integration errors are obvious.
foreach(dep taitank glog::glog gflags freetype)
  if(NOT TARGET ${dep})
    message(FATAL_ERROR
      "NeoFlux: required dependency target '${dep}' is missing. "
      "Add thirdparty via add_subdirectory(thirdparty) before this file, "
      "or provide ${dep} through find_package.")
  endif()
endforeach()

target_link_libraries(neoflux
  PUBLIC
    taitank
    glog::glog
    gflags
    freetype
    OpenGL::GL
)

# tgfx is an optional rendering backend.
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

# ---------------------------------------------------------------------------
# Copy third-party runtime DLLs next to the executable on Windows (standard
# Windows deployment model; no launcher scripts or PATH hacks required).
# ---------------------------------------------------------------------------
if(WIN32 AND TARGET glog::glog)
  add_custom_command(TARGET neoflux POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:glog::glog>"
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/"
    COMMENT "Copying glog runtime to bin/"
    ERROR_QUIET)
endif()

# ---------------------------------------------------------------------------
# Copy fonts from the repo assets/fonts/ to <output>/assets/fonts/ at build
# time. Fonts are NOT committed to git; place your own .ttf/.otf/.ttc files
# under assets/fonts/ (see docs/guide/fonts.md) before building applications
# that render text.
# ---------------------------------------------------------------------------
set(NEOFLUX_FONT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../assets/fonts")
set(NEOFLUX_FONT_DST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets/fonts")
if(EXISTS "${NEOFLUX_FONT_SRC_DIR}")
  add_custom_command(TARGET neoflux POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${NEOFLUX_FONT_DST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${NEOFLUX_FONT_SRC_DIR}"
      "${NEOFLUX_FONT_DST_DIR}"
    COMMENT "Copying fonts to bin/assets/fonts/"
    ERROR_QUIET)
endif()

# ---------------------------------------------------------------------------
# libmpv: integrated media player backend (desktop only).
#
# Resolution order:
#   1. thirdparty/mpv-bundle/ -- prebuilt headers + import lib + DLL, obtained
#      by running scripts/download_libmpv.ps1 (Windows) or .sh (Unix). This is
#      the recommended "out of the box" path; the bundle is gitignored.
#   2. A prebuilt libmpv installed inside thirdparty/mpv/ (e.g. from a manual
#      build of the mpv submodule).
#   3. System libmpv via find_package(mpv) or pkg-config.
# If only headers are present (no import lib / shared library), NEOFLUX_HAS_MPV
# stays undefined and the media widget compiles as a no-op with a warning.
# ---------------------------------------------------------------------------
option(NEOFLUX_USE_MPV "Enable libmpv media player backend" ON)
if(NEOFLUX_USE_MPV AND NOT ANDROID AND NOT IOS)
  set(NEOFLUX_MPV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/mpv")
  set(NEOFLUX_MPV_BUNDLE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/mpv-bundle")
  set(NEOFLUX_MPV_FOUND FALSE)
  foreach(mpv_root IN ITEMS "${NEOFLUX_MPV_BUNDLE_DIR}" "${NEOFLUX_MPV_DIR}")
    if(EXISTS "${mpv_root}/include/mpv/client.h")
      set(NEOFLUX_MPV_FOUND TRUE)
      target_include_directories(neoflux PRIVATE "${mpv_root}/include")
      if(WIN32)
        if(EXISTS "${mpv_root}/libmpv.dll.a")
          target_link_libraries(neoflux PRIVATE "${mpv_root}/libmpv.dll.a")
          target_compile_definitions(neoflux PUBLIC NEOFLUX_HAS_MPV=1)
          message(STATUS "NeoFlux: libmpv media backend enabled (bundle, MinGW import lib)")
        elseif(EXISTS "${mpv_root}/mpv.lib")
          target_link_libraries(neoflux PRIVATE "${mpv_root}/mpv.lib")
          target_compile_definitions(neoflux PUBLIC NEOFLUX_HAS_MPV=1)
          message(STATUS "NeoFlux: libmpv media backend enabled (bundle, MSVC import lib)")
        else()
          message(WARNING "NeoFlux: libmpv headers found but no import lib. Run scripts/download_libmpv.ps1 to fetch the prebuilt bundle; media widget is disabled for now.")
        endif()
        if(EXISTS "${mpv_root}/libmpv-2.dll")
          add_custom_command(TARGET neoflux POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${mpv_root}/libmpv-2.dll"
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libmpv-2.dll"
            COMMENT "Copying libmpv-2.dll to bin/")
        endif()
      else()
        if(EXISTS "${mpv_root}/libmpv.so")
          target_link_libraries(neoflux PRIVATE "${mpv_root}/libmpv.so")
          target_compile_definitions(neoflux PUBLIC NEOFLUX_HAS_MPV=1)
          message(STATUS "NeoFlux: libmpv media backend enabled (bundle, .so)")
        elseif(EXISTS "${mpv_root}/libmpv.dylib")
          target_link_libraries(neoflux PRIVATE "${mpv_root}/libmpv.dylib")
          target_compile_definitions(neoflux PUBLIC NEOFLUX_HAS_MPV=1)
          message(STATUS "NeoFlux: libmpv media backend enabled (bundle, .dylib)")
        else()
          message(WARNING "NeoFlux: libmpv headers found but no shared library. Install libmpv via your package manager; media widget is disabled for now.")
        endif()
      endif()
      break()
    endif()
  endforeach()
  if(NOT NEOFLUX_MPV_FOUND)
    find_package(mpv QUIET)
    if(mpv_FOUND)
      target_link_libraries(neoflux PRIVATE mpv::mpv)
      target_compile_definitions(neoflux PUBLIC NEOFLUX_HAS_MPV=1)
      message(STATUS "NeoFlux: libmpv media backend enabled (system)")
    else()
      find_package(PkgConfig QUIET)
      if(PkgConfig_FOUND)
        pkg_check_modules(MPV QUIET mpv)
        if(MPV_FOUND)
          target_link_libraries(neoflux PRIVATE ${MPV_LIBRARIES})
          target_include_directories(neoflux PRIVATE ${MPV_INCLUDE_DIRS})
          target_compile_definitions(neoflux PUBLIC NEOFLUX_HAS_MPV=1)
          message(STATUS "NeoFlux: libmpv media backend enabled (pkg-config)")
        else()
          message(STATUS "NeoFlux: libmpv not found; media widget will be non-functional. Run scripts/download_libmpv.ps1 (or .sh) to fetch the prebuilt bundle.")
        endif()
      else()
        message(STATUS "NeoFlux: libmpv not found; media widget will be non-functional. Run scripts/download_libmpv.ps1 (or .sh) to fetch the prebuilt bundle.")
      endif()
    endif()
  endif()
endif()

target_compile_features(neoflux PUBLIC cxx_std_20)

# Strict warnings for framework code only (not third-party deps).
# -Wno-error=attributes: glog headers emit dllimport attribute warnings on
# Windows; warning is still shown, just not fatal.
if(MSVC)
  target_compile_options(neoflux PRIVATE /W4 /permissive-)
else()
  target_compile_options(neoflux PRIVATE
    -Wall -Wextra -Wpedantic -Werror -Wno-conversion -Wno-sign-conversion
    -Wno-error=attributes
  )
endif()

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
# Tests (gtest; TDD workflow: write a failing test, implement, verify)
# ---------------------------------------------------------------------------
if(NEOFLUX_BUILD_TESTS)
  enable_testing()
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

install(DIRECTORY include/neoflux DESTINATION include)
