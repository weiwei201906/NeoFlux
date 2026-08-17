# =============================================================================
# NeoFlux - Font Demo
# =============================================================================

add_executable(font_demo main.cpp)
target_link_libraries(font_demo PRIVATE neoflux)
set_target_properties(font_demo PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
