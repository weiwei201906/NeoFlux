@echo off
REM Configure NeoFlux with the tgfx renderer under MSVC + Ninja.
REM Local dep sources are reused via FETCHCONTENT_SOURCE_DIR to avoid github fetches.
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul

set ROOT=C:\Users\99277\Desktop\NeoFlux-wt\tgfx
set BUILD=%ROOT%\build-tgfx

cmake -S %ROOT% -B %BUILD% -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DNEOFLUX_USE_TGFX=ON ^
  -DNEOFLUX_BUILD_TESTS=ON ^
  -DNEOFLUX_BUILD_EXAMPLES=ON ^
  -DFETCHCONTENT_SOURCE_DIR_TGFX=%ROOT%\thirdparty\tgfx ^
  -DFETCHCONTENT_SOURCE_DIR_GLOG=%ROOT%\thirdparty\glog ^
  -DFETCHCONTENT_SOURCE_DIR_GFLAGS=%ROOT%\thirdparty\gflags ^
  -DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=%ROOT%\thirdparty\googletest ^
  -DFETCHCONTENT_SOURCE_DIR_GLFW=%ROOT%\thirdparty\glfw ^
  -DFETCHCONTENT_SOURCE_DIR_TAITANK=%ROOT%\thirdparty\taitank ^
  -DFETCHCONTENT_SOURCE_DIR_FREETYPE=%ROOT%\thirdparty\freetype
echo CONFIGURE_EXIT=%ERRORLEVEL%
endlocal
