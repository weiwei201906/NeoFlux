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
