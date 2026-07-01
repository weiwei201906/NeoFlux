# GNU General Public License v3.0
# Author: weiwei201906
# Date: 2026-07-01

# Ensure subproject package configs from thirdparty are visible to other dependencies.
list(APPEND CMAKE_PREFIX_PATH
  "${CMAKE_BINARY_DIR}/thirdparty/gflags"
  "${CMAKE_BINARY_DIR}/thirdparty/googletest"
)
set(gflags_DIR "${CMAKE_BINARY_DIR}/thirdparty/gflags" CACHE PATH "Location of gflagsConfig.cmake" FORCE)
set(GTest_DIR "${CMAKE_BINARY_DIR}/thirdparty/googletest" CACHE PATH "Location of GTestConfig.cmake" FORCE)

# Use static dependency builds and avoid dependency subproject tests.
set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "Enable PIC for thirdparty dependencies" FORCE)
option(BUILD_TESTING "Build tests for dependencies" OFF)
set(BUILD_TESTING OFF CACHE BOOL "Disable dependency subproject tests" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries for thirdparty dependencies" FORCE)

if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/gflags/CMakeLists.txt")
  add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/gflags" EXCLUDE_FROM_ALL)
else()
  message(FATAL_ERROR "gflags not found in thirdparty. Run scripts/setup_thirdparty.sh to install submodules.")
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/glog/CMakeLists.txt")
  add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/glog" EXCLUDE_FROM_ALL)
else()
  message(FATAL_ERROR "glog not found in thirdparty. Run scripts/setup_thirdparty.sh to install submodules.")
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/googletest/CMakeLists.txt")
  add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/googletest" EXCLUDE_FROM_ALL)
else()
  message(FATAL_ERROR "googletest not found in thirdparty. Run scripts/setup_thirdparty.sh to install submodules.")
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/Taitank/CMakeLists.txt" OR EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/taitank/CMakeLists.txt")
  if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/Taitank/CMakeLists.txt")
    add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/Taitank" EXCLUDE_FROM_ALL)
  else()
    add_subdirectory("${CMAKE_SOURCE_DIR}/thirdparty/taitank" EXCLUDE_FROM_ALL)
  endif()
else()
  message(FATAL_ERROR "Taitank not found in thirdparty. Run scripts/setup_thirdparty.sh to install submodules.")
endif()

# Skia is expected under thirdparty/skia (can be a prebuilt stub)
if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/skia/include" AND EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/skia/lib/libskia.a")
  add_library(Skia::Skia STATIC IMPORTED)
  set_target_properties(Skia::Skia PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/thirdparty/skia/lib/libskia.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/thirdparty/skia/include"
  )
else()
  message(FATAL_ERROR "Skia not found under thirdparty/skia. Run scripts/setup_thirdparty.sh or provide a Skia build in thirdparty/skia.")
endif()
