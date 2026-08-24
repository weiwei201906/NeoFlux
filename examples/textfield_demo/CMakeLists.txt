# =============================================================================
# NeoFlux - TextField demo
#
# Demonstrates text input, cursor navigation, and keyboard focus.
# =============================================================================

add_executable(textfield_demo main.cpp)

target_link_libraries(textfield_demo
  PRIVATE
    neoflux
    glog::glog
    gflags
)

target_include_directories(textfield_demo PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

if(WIN32 AND MINGW)
  target_link_options(textfield_demo PRIVATE -mwindows)
endif()
