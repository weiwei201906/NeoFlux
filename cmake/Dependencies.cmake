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

# Disable dependency subproject tests to avoid requiring extra external test packages.
option(BUILD_TESTING "Build tests for dependencies" OFF)
set(BUILD_TESTING OFF CACHE BOOL "Disable dependency subproject tests" FORCE)

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
