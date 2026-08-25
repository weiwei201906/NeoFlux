# =============================================================================
# NeoFlux unit tests
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
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
  )
  gtest_discover_tests(${test_name})
endfunction()

neoflux_add_test(ring_queue_test ring_queue_test.cpp)
neoflux_add_test(ring_queue_stress_test ring_queue_stress_test.cpp)
neoflux_add_test(widget_test widget_test.cpp)
neoflux_add_test(widget_tree_stress_test widget_tree_stress_test.cpp)
neoflux_add_test(route_registry_test route_registry_test.cpp)
neoflux_add_test(render_command_test render_command_test.cpp)
