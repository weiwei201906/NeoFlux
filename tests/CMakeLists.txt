# GNU General Public License v3.0
# Author: weiwei201906
# Date: 2026-07-01
add_executable(neoflux_test
  "src/test_main.cpp"
  "src/test_placeholder.cpp"
)

target_link_libraries(neoflux_test
  PRIVATE neoflux_core
  PRIVATE gtest_main
  PRIVATE glog::glog
)

target_compile_features(neoflux_test PRIVATE cxx_std_20)

enable_testing()
add_test(NAME neoflux_test COMMAND neoflux_test)
