# GNU General Public License v3.0
# Author: weiwei201906
# Date: 2026-07-14

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/glog/CMakeLists.txt")
  add_subdirectory(glog)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gflags/CMakeLists.txt")
  add_subdirectory(gflags)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/googletest/CMakeLists.txt")
  add_subdirectory(googletest)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/taitank/CMakeLists.txt")
  add_subdirectory(taitank)
endif()
