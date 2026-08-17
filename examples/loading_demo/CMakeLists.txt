# =============================================================================
# NeoFlux - Loading Animation Demo
# =============================================================================

add_executable(loading_demo main.cpp)
target_link_libraries(loading_demo PRIVATE neoflux)
set_target_properties(loading_demo PROPERTIES RUNTIME_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/bin")
