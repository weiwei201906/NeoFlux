# GNU General Public License v3.0
# Author: weiwei201906
# Date: 2026-07-01
find_path(Skia_INCLUDE_DIR
  NAMES core/SkCanvas.h
  PATHS "${CMAKE_SOURCE_DIR}/thirdparty/skia/include"
  NO_DEFAULT_PATH
)

find_library(Skia_LIBRARY
  NAMES skia
  PATHS "${CMAKE_SOURCE_DIR}/thirdparty/skia/lib"
  NO_DEFAULT_PATH
)

if(NOT Skia_INCLUDE_DIR OR NOT Skia_LIBRARY)
  message(FATAL_ERROR "Skia not found.")
endif()

add_library(Skia::Skia STATIC IMPORTED)
set_target_properties(Skia::Skia PROPERTIES
  IMPORTED_LOCATION "${Skia_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES "${Skia_INCLUDE_DIR}"
)
