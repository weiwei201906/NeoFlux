# =============================================================================
# NeoFlux framework unit tests
# =============================================================================

include(GoogleTest)

# Public framework headers live under neoflux/include (inherited via the
# `neoflux` target's PUBLIC include dir too, but set explicitly for clarity).
set(NEOFLUX_TEST_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/neoflux/include")

# Helper function to add a test target.
function(neoflux_add_test test_name)
  add_executable(${test_name} ${ARGN})
  target_link_libraries(${test_name}
    PRIVATE
      neoflux
      gtest
      gtest_main
      gmock
  )
  target_include_directories(${test_name} PRIVATE
    ${NEOFLUX_TEST_INCLUDE_DIR}
  )
  gtest_discover_tests(${test_name})
endfunction()

neoflux_add_test(ring_queue_test ring_queue_test.cpp)
neoflux_add_test(widget_test widget_test.cpp)
neoflux_add_test(route_registry_test route_registry_test.cpp)
neoflux_add_test(render_command_test render_command_test.cpp)
neoflux_add_test(widget_extra_test widget_extra_test.cpp)
neoflux_add_test(button_e2e_test button_e2e_test.cpp)

# ---------------------------------------------------------------------------
# libmpv media player integration test (real sample.mp4 playback).
#
# Requires a GL context, so the test creates a hidden GLFW window itself and
# links glfw explicitly. NEOFLUX_HAS_MPV is propagated from the neoflux target
# (INTERFACE) when the mpv bundle was found. The real test video path is
# injected at configure time. libmpv-2.dll is copied next to the executable so
# the test can load it at runtime.
# ---------------------------------------------------------------------------
if(NEOFLUX_MPV_FOUND)
  set(MEDIA_TEST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/data/sample.mp4")

  # Copy libmpv-2.dll into the runtime bin directory at configure time as well,
  # so the DLL is already next to the executable when gtest_discover_tests runs
  # the test binary during the build (the POST_BUILD copy below runs too late
  # for that first launch and would fail with STATUS_DLL_NOT_FOUND).
  # On Linux, mpv is a system library resolved via ld.so, nothing to copy.
  if(WIN32)
    file(COPY "${MPV_DLL}" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
  endif()

  add_executable(mpv_media_player_test mpv_media_player_test.cpp)
  target_link_libraries(mpv_media_player_test
    PRIVATE
      neoflux
      gtest
      gtest_main
      gmock
      glfw
  )
  target_include_directories(mpv_media_player_test PRIVATE
    ${NEOFLUX_TEST_INCLUDE_DIR}
  )
  target_compile_definitions(mpv_media_player_test PRIVATE
    NEOFLUX_TEST_MEDIA_PATH="${MEDIA_TEST_FILE}"
  )
  gtest_discover_tests(mpv_media_player_test)

  # The Windows loader resolves libmpv-2.dll from the executable directory.
  if(WIN32)
    add_custom_command(TARGET mpv_media_player_test POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "${MPV_DLL}" "$<TARGET_FILE_DIR:mpv_media_player_test>"
      COMMENT "Copying libmpv-2.dll next to the mpv test executable"
    )
  endif()
endif()
