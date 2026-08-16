#!/usr/bin/env bash
# =============================================================================
# NeoFlux - Font Download Script (Linux / macOS)
#
# Downloads open-source fonts into thirdparty/fonts/ for out-of-the-box
# text rendering. Run once after cloning the repository.
#
# Usage:
#   chmod +x download_fonts.sh
#   ./download_fonts.sh
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "NeoFlux font downloader"
echo "Target directory: ${SCRIPT_DIR}"

# ---------------------------------------------------------------------------
# Font definitions: name, output filename, download URL(s) in priority order.
# ---------------------------------------------------------------------------
download_font() {
    local name="$1"
    local filename="$2"
    shift 2
    local urls=("$@")

    local outpath="${SCRIPT_DIR}/${filename}"
    if [[ -f "${outpath}" ]]; then
        local size
        size=$(wc -c < "${outpath}" | tr -d ' ')
        if [[ "${size}" -gt 1000 ]]; then
            echo "[SKIP] ${name} already exists ($(( size / 1024 )) KB)"
            return 0
        fi
    fi

    echo "[DOWN] ${name}"
    local success=0
    for url in "${urls[@]}"; do
        for attempt in 1 2 3; do
            echo "  Attempt ${attempt}/3: ${url}"
            if curl -fL --connect-timeout 30 --max-time 120 -o "${outpath}" "${url}" 2>/dev/null; then
                local size
                size=$(wc -c < "${outpath}" | tr -d ' ')
                if [[ "${size}" -gt 1000 ]]; then
                    echo "  OK: ${filename} ($(( size / 1024 )) KB)"
                    success=1
                    break 2
                fi
                echo "  Downloaded file too small, retrying..."
            else
                echo "  Failed (curl exit code: $?)"
            fi
        done
    done

    if [[ "${success}" -eq 0 ]]; then
        echo "  FAILED: Could not download ${name}"
        echo "  Please manually place a .ttf/.otf font in: ${SCRIPT_DIR}"
        rm -f "${outpath}"
        return 1
    fi
    return 0
}

downloaded=0
skipped=0
failed=0

# Noto Sans SC (CJK support, variable font)
if download_font "Noto Sans SC Regular (CJK)" "NotoSansSC-Regular.ttf" \
    "https://github.com/google/fonts/raw/main/ofl/notosanssc/NotoSansSC%5Bwght%5D.ttf" \
    "https://raw.githubusercontent.com/google/fonts/main/ofl/notosanssc/NotoSansSC%5Bwght%5D.ttf"; then
    if [[ -f "${SCRIPT_DIR}/NotoSansSC-Regular.ttf" ]]; then
        downloaded=$((downloaded + 1))
    else
        skipped=$((skipped + 1))
    fi
else
    failed=$((failed + 1))
fi

# DejaVu Sans (Latin fallback)
if download_font "DejaVu Sans (Latin fallback)" "DejaVuSans.ttf" \
    "https://github.com/dejavu-fonts/dejavu-fonts/raw/master/ttf/DejaVuSans.ttf" \
    "https://raw.githubusercontent.com/dejavu-fonts/dejavu-fonts/master/ttf/DejaVuSans.ttf"; then
    if [[ -f "${SCRIPT_DIR}/DejaVuSans.ttf" ]]; then
        downloaded=$((downloaded + 1))
    else
        skipped=$((skipped + 1))
    fi
else
    failed=$((failed + 1))
fi

echo ""
echo "Summary: ${downloaded} downloaded, ${skipped} skipped, ${failed} failed"
if [[ "${failed}" -gt 0 ]]; then
    echo "Note: NeoFlux will fall back to system fonts if bundled fonts are missing."
fi
