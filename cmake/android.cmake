# =============================================================================
# NeoFlux - Android NDK build configuration
#
# This file is used when building NeoFlux as part of an Android NDK project.
# It is included by the app's build.gradle via externalNativeBuild.
#
# Usage in build.gradle:
#   android {
#     externalNativeBuild {
#       cmake {
#         path "src/main/cpp/CMakeLists.txt"
#       }
#     }
#   }
#
# In the app's CMakeLists.txt:
#   add_subdirectory(thirdparty/neoflux)
#   target_link_libraries(my_app PRIVATE neoflux)
# =============================================================================

# Android-specific configuration for NeoFlux.
# This block is only active when CMAKE_SYSTEM_NAME equals Android.
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  # Enable Android-specific platform macros.
  target_compile_definitions(neoflux PUBLIC
    NEOFLUX_PLATFORM_MOBILE=1
    NEOFLUX_PLATFORM_ANDROID=1
  )

  # Link against Android NDK libraries.
  target_link_libraries(neoflux PUBLIC
    android       # Android native app glue
    log           # Android logging (used by glog fallback)
    EGL           # EGL for surface management
    GLESv3        # OpenGL ES 3.0 (fallback to GLESv2 if unavailable)
  )

  # Android uses the mobile bridge (EGL + ANativeWindow) instead of GLFW.
  # The mobile bridge source is already added in the root CMakeLists.

  # Disable libmpv on Android (use AndroidMediaPlayer instead).
  set(NEOFLUX_USE_MPV OFF CACHE BOOL "" FORCE)

  message(STATUS "NeoFlux: Android build configured (NDK ${ANDROID_NDK_VERSION})")
endif()
