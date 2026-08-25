#!/usr/bin/env bash
# =============================================================================
# NeoFlux - Download Fonts (Linux / macOS)
#
# Downloads the default NotoSansSC font (SIL OFL 1.1) to thirdparty/fonts/.
# Run this before building examples if you need CJK text rendering.
#
# Usage:
#   bash examples/download_fonts.sh
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FONT_DIR="${SCRIPT_DIR}/../thirdparty/fonts"
FONT_FILE="${FONT_DIR}/NotoSansSC-Regular.otf"
FONT_URL="https://github.com/googlefonts/noto-cjk/raw/main/Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf"

mkdir -p "${FONT_DIR}"

if [ -f "${FONT_FILE}" ]; then
    echo "Font already exists: ${FONT_FILE}"
    exit 0
fi

echo "Downloading NotoSansSC-Regular.otf..."
echo "  URL: ${FONT_URL}"
echo "  Dest: ${FONT_FILE}"

if command -v curl &> /dev/null; then
    curl -L -o "${FONT_FILE}" "${FONT_URL}"
elif command -v wget &> /dev/null; then
    wget -O "${FONT_FILE}" "${FONT_URL}"
else
    echo "Error: neither curl nor wget found." >&2
    echo "Please download manually from:" >&2
    echo "  ${FONT_URL}" >&2
    echo "and save to: ${FONT_FILE}" >&2
    exit 1
fi

SIZE=$(du -h "${FONT_FILE}" | cut -f1)
echo "Downloaded: ${SIZE}"
echo "Done. Font ready for use."
