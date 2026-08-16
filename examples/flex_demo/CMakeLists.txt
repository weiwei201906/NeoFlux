# =============================================================================
# NeoFlux - Flex Layout Demo
# =============================================================================

add_executable(flex_demo main.cpp)
target_link_libraries(flex_demo PRIVATE neoflux)
set_target_properties(flex_demo PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Copy fonts to the output directory.
if(EXISTS "${CMAKE_SOURCE_DIR}/thirdparty/fonts/NotoSansSC-Regular.ttf")
  add_custom_command(TARGET flex_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_SOURCE_DIR}/thirdparty/fonts"
      "$<TARGET_FILE_DIR:flex_demo>/fonts"
    COMMENT "Copying fonts to flex_demo output directory"
  )
endif()
