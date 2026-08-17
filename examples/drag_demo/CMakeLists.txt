add_executable(drag_demo main.cpp)
target_link_libraries(drag_demo PRIVATE neoflux)
set_target_properties(drag_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
