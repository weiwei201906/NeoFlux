# =============================================================================
# NeoFlux - iOS build configuration
#
# This file is used when building NeoFlux for iOS (via CMake + Xcode or
# toolchain file). It configures platform-specific frameworks and compiler
# flags.
#
# Usage with toolchain:
#   cmake -DCMAKE_TOOLCHAIN_FILE=ios.toolchain.cmake -DPLATFORM=OS64 ..
#
# Or with Xcode generation:
#   cmake -G Xcode -DCMAKE_SYSTEM_NAME=iOS ..
# =============================================================================

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
  # Enable iOS-specific platform macros.
  target_compile_definitions(neoflux PUBLIC
    NEOFLUX_PLATFORM_MOBILE=1
    NEOFLUX_PLATFORM_IOS=1
  )

  # Link against iOS frameworks.
  target_link_libraries(neoflux PUBLIC
    "-framework UIKit"
    "-framework Foundation"
    "-framework QuartzCore"
    "-framework AVFoundation"      # AVPlayer for media playback
    "-framework CoreVideo"         # CVPixelBuffer for texture conversion
    "-framework OpenGLES"          # EAGL context + GLES 2.0/3.0
  )

  # Enable Objective-C++ for files that need to interact with UIKit/AVFoundation.
  set_source_files_properties(
    src/media/ios/ios_media_player.mm
    PROPERTIES COMPILE_FLAGS "-fobjc-arc"
  )

  # iOS uses the mobile bridge (EAGL context + UIKit view).
  # The mobile bridge source is already added in the root CMakeLists.

  # Disable libmpv on iOS (use IosMediaPlayer with AVPlayer instead).
  set(NEOFLUX_USE_MPV OFF CACHE BOOL "" FORCE)

  # iOS bitcode is deprecated in Xcode 14+, disable it.
  set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE NO)

  message(STATUS "NeoFlux: iOS build configured")
endif()
