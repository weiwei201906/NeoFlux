# =============================================================================
# NeoFlux examples
# =============================================================================

# On Windows with MinGW, link as GUI subsystem to avoid a console window.
# Logs go to files by default (glog), so no stdout/stderr console is needed.
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  add_link_options(-mwindows)
endif()

add_subdirectory(hello_neoflux)
add_subdirectory(counter)
add_subdirectory(flex_demo)
add_subdirectory(font_demo)
add_subdirectory(scroll_demo)
add_subdirectory(loading_demo)
add_subdirectory(drag_demo)
