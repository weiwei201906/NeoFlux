# =============================================================================
# NeoFlux - Media Player Demo
#
# Only built when libmpv was found at configure time (NEOFLUX_MPV_FOUND).
# On platforms without mpv, this example is silently skipped.
# =============================================================================

if(NEOFLUX_MPV_FOUND)
  add_executable(media_player_demo main.cpp)
  target_link_libraries(media_player_demo PRIVATE neoflux)
  set_target_properties(media_player_demo PROPERTIES RUNTIME_OUTPUT_DIRECTORY
      "${CMAKE_BINARY_DIR}/bin")
else()
  message(STATUS "libmpv not found; skipping media_player_demo example")
endif()
