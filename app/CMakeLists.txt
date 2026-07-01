# GNU General Public License v3.0
# Author: weiwei201906
# Date: 2026-07-01
add_executable(neoflux_app "src/main.cpp" "src/MyAppWidget.cpp")

target_link_libraries(neoflux_app
  PRIVATE neoflux_core
  PRIVATE glog::glog
)

target_include_directories(neoflux_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_features(neoflux_app PRIVATE cxx_std_20)
