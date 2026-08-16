# =============================================================================
# NeoFlux - Counter example
#
# Minimal counter application demonstrating StatefulWidget, Button callbacks,
# and flex layout.
# =============================================================================

add_executable(counter main.cpp)

target_link_libraries(counter
  PRIVATE
    neoflux
    glog::glog
    gflags
)

target_include_directories(counter PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

# On Windows with MinGW, -mwindows produces a GUI subsystem executable that
# does not spawn a console window.
if(WIN32 AND MINGW)
  target_link_options(counter PRIVATE -mwindows)
endif()
