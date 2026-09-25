# =============================================================================
# NeoFlux unit tests
#
# TDD workflow: write a failing test, implement the feature, then verify with
# `ctest`. Tests link against the neoflux library and gtest.
# =============================================================================

include(GoogleTest)

# Helper function to add a test target.
function(neoflux_add_test test_name)
  add_executable(${test_name} ${ARGN})
  target_link_libraries(${test_name}
    PRIVATE
      neoflux
      gtest
      gtest_main
      gmock
  )
  target_include_directories(${test_name} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
  )
  gtest_discover_tests(${test_name})
endfunction()

neoflux_add_test(ring_queue_test ring_queue_test.cpp)
neoflux_add_test(ring_queue_stress_test ring_queue_stress_test.cpp)
neoflux_add_test(widget_test widget_test.cpp)
neoflux_add_test(widget_tree_stress_test widget_tree_stress_test.cpp)
neoflux_add_test(object_pool_test object_pool_test.cpp)
neoflux_add_test(backpressure_stress_test backpressure_stress_test.cpp)
neoflux_add_test(route_registry_test route_registry_test.cpp)
neoflux_add_test(render_command_test render_command_test.cpp)

# libmpv media player tests (skipped automatically when NEOFLUX_HAS_MPV is
# undefined, i.e. no libmpv import lib was found at configure time).
neoflux_add_test(mpv_media_player_test mpv_media_player_test.cpp)
target_link_libraries(mpv_media_player_test PRIVATE glfw)
# Path of the bundled 2s test clip used by the playback smoke tests.
target_compile_definitions(mpv_media_player_test PRIVATE
  NEOFLUX_TEST_MEDIA_PATH="${CMAKE_CURRENT_SOURCE_DIR}/data/sample.mp4")
