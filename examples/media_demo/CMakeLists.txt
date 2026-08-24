# =============================================================================
# NeoFlux - Media player demo
#
# Demonstrates MediaWidget backed by ffplay subprocess.
# Requires ffplay on the system PATH.
# =============================================================================

add_executable(media_demo main.cpp)

target_link_libraries(media_demo
  PRIVATE
    neoflux
    glog::glog
    gflags
)

target_include_directories(media_demo PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

if(WIN32 AND MINGW)
  target_link_options(media_demo PRIVATE -mwindows)
endif()
