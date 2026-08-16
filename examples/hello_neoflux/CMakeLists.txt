# =============================================================================
# NeoFlux - Hello World example
# =============================================================================

add_executable(hello_neoflux main.cpp)

target_link_libraries(hello_neoflux
  PRIVATE
    neoflux
    glog::glog
    gflags
)

target_include_directories(hello_neoflux PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

# On Windows with MinGW, -mwindows produces a GUI subsystem executable that
# does not spawn a console window. Logs go to --log_dir by default.
if(WIN32 AND MINGW)
  target_link_options(hello_neoflux PRIVATE -mwindows)
endif()
