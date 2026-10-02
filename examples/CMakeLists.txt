# =============================================================================
# NeoFlux examples
# =============================================================================

# Build examples with the console subsystem on Windows so their `int main()`
# entry point resolves (MSVC /subsystem:windows expects WinMain). A console
# window also surfaces glog output during development.
set(CMAKE_WIN32_EXECUTABLE OFF)

add_subdirectory(hello_neoflux)
add_subdirectory(counter)
add_subdirectory(flex_demo)
add_subdirectory(font_demo)
add_subdirectory(scroll_demo)
add_subdirectory(loading_demo)
add_subdirectory(drag_demo)
add_subdirectory(media_player)
