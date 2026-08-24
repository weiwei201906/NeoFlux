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
