# =============================================================================
# NeoFlux Android Example - CMakeLists.txt
#
# Builds the native library for the Android example. Includes NeoFlux as a
# subdirectory and links it against the example native code.
# =============================================================================

cmake_minimum_required(VERSION 3.22.1)
project(neoflux_example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# NeoFlux is expected to be checked out at ../../../../thirdparty/neoflux
# relative to this file (app/src/main/cpp/CMakeLists.txt).
# Adjust the path if your project layout differs.
set(NEOFLUX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../thirdparty/neoflux")

if(NOT EXISTS "${NEOFLUX_DIR}/CMakeLists.txt")
  message(FATAL_ERROR
    "NeoFlux not found at ${NEOFLUX_DIR}. "
    "Please clone it to thirdparty/neoflux or update NEOFLUX_DIR.")
endif()

# Disable examples and tests when building as a dependency.
set(NEOFLUX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(NEOFLUX_BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(${NEOFLUX_DIR} neoflux_build)

# Build the example native library.
add_library(neoflux_example SHARED
  native-lib.cpp
)

target_link_libraries(neoflux_example PRIVATE
  neoflux
  android
  log
)
