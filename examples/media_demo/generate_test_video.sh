#!/usr/bin/env bash
# =============================================================================
# NeoFlux - Generate test video for media_demo
#
# Uses ffmpeg to generate a short test video with color bars and a
# sine wave audio tone. Output: test_video.mp4 in the same directory.
#
# Requirements: ffmpeg must be on the system PATH.
#   Ubuntu/Debian: sudo apt install ffmpeg
#   macOS:         brew install ffmpeg
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT="${SCRIPT_DIR}/test_video.mp4"

echo "Generating test video: ${OUTPUT}"
echo

ffmpeg -y \
  -f lavfi -i testsrc=duration=10:size=640x360:rate=30 \
  -f lavfi -i sine=frequency=440:duration=10 \
  -c:v libx264 -preset fast -crf 23 \
  -c:a aac -b:a 128k \
  -pix_fmt yuv420p \
  "${OUTPUT}"

echo
echo "Test video generated: ${OUTPUT}"
echo "Run media_demo and enter the path above in the text field."
