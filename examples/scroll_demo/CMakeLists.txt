# =============================================================================
# NeoFlux - Scroll View Demo
# =============================================================================

add_executable(scroll_demo main.cpp)
target_link_libraries(scroll_demo PRIVATE neoflux)
set_target_properties(scroll_demo PROPERTIES RUNTIME_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/bin")
