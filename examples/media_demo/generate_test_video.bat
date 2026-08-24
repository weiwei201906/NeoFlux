@echo off
REM =============================================================================
REM NeoFlux - Generate test video for media_demo
REM
REM Uses ffmpeg to generate a short test video with color bars and a
REM sine wave audio tone. Output: test_video.mp4 in the same directory.
REM
REM Requirements: ffmpeg must be on the system PATH.
REM =============================================================================

setlocal
set OUTPUT=%~dp0test_video.mp4

echo Generating test video: %OUTPUT%
echo.

ffmpeg -y ^
  -f lavfi -i testsrc=duration=10:size=640x360:rate=30 ^
  -f lavfi -i sine=frequency=440:duration=10 ^
  -c:v libx264 -preset fast -crf 23 ^
  -c:a aac -b:a 128k ^
  -pix_fmt yuv420p ^
  "%OUTPUT%"

if %ERRORLEVEL% equ 0 (
  echo.
  echo Test video generated: %OUTPUT%
  echo Run media_demo and enter the path above in the text field.
) else (
  echo.
  echo Failed to generate test video. Ensure ffmpeg is installed and on PATH.
)

endlocal
pause
