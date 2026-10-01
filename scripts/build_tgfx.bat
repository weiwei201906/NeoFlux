@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
set BUILD=C:\Users\99277\Desktop\NeoFlux-wt\tgfx\build-tgfx
cmake --build %BUILD% %*
echo BUILD_EXIT=%ERRORLEVEL%
endlocal
