# =============================================================================
# NeoFlux examples
# =============================================================================

# Build examples as GUI subsystem on Windows (no console window).
set(CMAKE_WIN32_EXECUTABLE ON)

add_subdirectory(hello_neoflux)
add_subdirectory(counter)
add_subdirectory(flex_demo)
add_subdirectory(font_demo)
add_subdirectory(scroll_demo)
add_subdirectory(loading_demo)
add_subdirectory(drag_demo)
add_subdirectory(textfield_demo)
add_subdirectory(media_demo)

# On Linux/macOS, set rpath to $ORIGIN/lib so shared libs are found relative
# to the executable. On Windows, DLLs are copied next to the exe by the
# neoflux POST_BUILD step, so no rpath or launcher script is needed.
if(NOT WIN32)
  set(NEOFLUX_EXAMPLE_TARGETS
    hello_neoflux counter flex_demo font_demo scroll_demo
    loading_demo drag_demo textfield_demo media_demo
  )
  foreach(example_target ${NEOFLUX_EXAMPLE_TARGETS})
    set_target_properties(${example_target} PROPERTIES
      BUILD_RPATH "$ORIGIN/lib"
      INSTALL_RPATH "$ORIGIN/lib"
    )
  endforeach()
endif()
