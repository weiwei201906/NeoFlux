# GNU General Public License v3.0
# Author: weiwei201906
# Date: 2026-07-01
add_library(neoflux_core STATIC)

file(GLOB_RECURSE neoflux_core_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

target_sources(neoflux_core PRIVATE ${neoflux_core_sources})
target_include_directories(neoflux_core
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

find_package(Threads REQUIRED)

target_link_libraries(neoflux_core
  PUBLIC glog::glog
  PUBLIC taitank
  PUBLIC Skia::Skia
  PUBLIC Threads::Threads
)

target_compile_options(neoflux_core PRIVATE
  -Wall -Wextra -Wpedantic
)

target_compile_features(neoflux_core PUBLIC cxx_std_20)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.20")
  include(CheckIPOSupported)
  check_ipo_supported(RESULT ipo_supported)
  if(ipo_supported)
    set_property(TARGET neoflux_core PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
  endif()
endif()
