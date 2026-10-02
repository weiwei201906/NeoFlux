# ---------------------------------------------------------------------------
# NeoFlux - cmake/DownloadMPV.cmake
#
# Locates or fetches libmpv for the desktop media backend.
#
# On Windows: if thirdparty/mpv-bundle is missing, invoke
# scripts/download_mpv.ps1 to pull the latest shinchiro/mpv-win32 build
# (headers + libmpv-2.dll). The MSVC import library (mpv.lib) is generated
# from the DLL by neoflux/CMakeLists.txt using dumpbin/lib.exe.
#
# On Linux: rely on the system package (libmpv-dev via pkg-config).
#
# Sets:
#   MPV_BUNDLE_DIR   path to thirdparty/mpv-bundle (Windows)
#   MPV_DLL          path to libmpv-2.dll (Windows)
#   MPV_IMPORTED_LIB path to mpv.lib (Windows, generated on first configure)
#   NEOFLUX_MPV_FOUND TRUE if mpv is available
# ---------------------------------------------------------------------------

set(MPV_BUNDLE_DIR "${CMAKE_SOURCE_DIR}/thirdparty/mpv-bundle")

if(WIN32)
  if(NOT EXISTS "${MPV_BUNDLE_DIR}/include/mpv/client.h" OR
     NOT EXISTS "${MPV_BUNDLE_DIR}/libmpv-2.dll")
    message(STATUS "libmpv bundle missing; fetching latest Windows build...")
    find_program(POWERSHELL_EXECUTABLE NAMES powershell pwsh)
    if(NOT POWERSHELL_EXECUTABLE)
      message(WARNING "powershell not found; cannot auto-download libmpv. "
                      "Run scripts/download_mpv.ps1 manually.")
    else()
      execute_process(
        COMMAND ${POWERSHELL_EXECUTABLE} -ExecutionPolicy Bypass
                -File "${CMAKE_SOURCE_DIR}/scripts/download_mpv.ps1"
                -RepoRoot "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE MPV_DOWNLOAD_RESULT
        ERROR_VARIABLE MPV_DOWNLOAD_ERR)
      if(NOT MPV_DOWNLOAD_RESULT EQUAL 0)
        message(WARNING "libmpv auto-download failed: ${MPV_DOWNLOAD_ERR}")
      endif()
    endif()
  endif()

  set(MPV_DLL "${MPV_BUNDLE_DIR}/libmpv-2.dll")
  set(MPV_IMPORTED_LIB "${MPV_BUNDLE_DIR}/mpv.lib")
else()
  # Linux/macOS: use pkg-config to find system libmpv.
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(MPV QUIET mpv)
  endif()
endif()
