# =============================================================================
# NeoFlux examples
# =============================================================================

# Build examples as GUI subsystem on Windows (no console window).
# CMAKE_WIN32_EXECUTABLE works on both MSVC and MinGW. On non-Windows
# platforms this variable has no effect.
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

# On Windows, third-party DLLs (glog, libmpv) live in bin/lib/. Generate a
# launcher .bat for each example that prepends lib/ to PATH before starting.
# MinGW ld does not support --delay-load, so this keeps DLLs out of the exe
# directory while remaining directly runnable via the .bat launcher.
if(WIN32)
  set(NEOFLUX_EXAMPLE_TARGETS
    hello_neoflux counter flex_demo font_demo scroll_demo
    loading_demo drag_demo textfield_demo media_demo
  )
  foreach(example_target ${NEOFLUX_EXAMPLE_TARGETS})
    file(WRITE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${example_target}.bat"
      "@echo off\r\nset PATH=%~dp0lib;%PATH%\r\n\"%~dp0${example_target}.exe\" %*\r\n")
  endforeach()
endif()
