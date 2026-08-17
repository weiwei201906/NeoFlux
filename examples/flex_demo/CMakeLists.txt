# =============================================================================
# NeoFlux - Flex Layout Demo
# =============================================================================

add_executable(flex_demo main.cpp)
target_link_libraries(flex_demo PRIVATE neoflux)
set_target_properties(flex_demo PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
