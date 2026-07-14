# GNU General Public License v3.0
# Author: weiwei201906
# Date: 2026-07-01
cmake_minimum_required(VERSION 3.20)
project(neoflux VERSION 1.0.0 LANGUAGES CXX)

function(find_package pkg_name)
  if(pkg_name STREQUAL "gflags")
    set(gflags_FOUND ON PARENT_SCOPE)
    set(gflags_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/thirdparty/gflags/src" PARENT_SCOPE)
    set(gflags_LIBRARIES gflags PARENT_SCOPE)
    set(gflags_LIBRARY gflags PARENT_SCOPE)
    return()
  endif()
  _find_package(${pkg_name} ${ARGN})
endfunction()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(cmake/Dependencies.cmake)
add_subdirectory(framework)
add_subdirectory(app)
add_subdirectory(tests)
