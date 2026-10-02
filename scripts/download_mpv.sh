#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# NeoFlux - scripts/download_mpv.sh
#
# Downloads the latest macOS libmpv build from the community mpv-release GitHub
# releases, extracts only what NeoFlux needs:
#   thirdparty/mpv-bundle/include/mpv/*.h
#   thirdparty/mpv-bundle/lib/libmpv.dylib
#
# On Linux, libmpv is normally installed via the system package manager
# (e.g. apt install libmpv-dev) and found by pkg-config; this script is only
# needed on macOS.
#
# Usage:
#   bash scripts/download_mpv.sh
# ---------------------------------------------------------------------------
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUNDLE_DIR="${REPO_ROOT}/thirdparty/mpv-bundle"

# Already present?
if [[ -f "${BUNDLE_DIR}/include/mpv/client.h" && \
      -f "${BUNDLE_DIR}/lib/libmpv.dylib" ]]; then
  echo "mpv bundle already present at ${BUNDLE_DIR}"
  exit 0
fi

mkdir -p "${BUNDLE_DIR}"

# Query latest mpv-macOS release (stefan-smolik/mpv-macos or zyxar/mpv-macos).
API="https://api.github.com/repos/zyxar/mpv-macos/releases/latest"
echo "Querying latest macOS mpv build from ${API}"
REL="$(curl -fsSL -H "User-Agent: neoflux" "${API}")"

# Pick the universal or arm64 .tar.gz asset.
ASSET_URL="$(echo "${REL}" | grep -o '"browser_download_url": *"[^"]*\.tar\.gz"' | \
  head -1 | sed 's/.*": *"//;s/"$//')"

if [[ -z "${ASSET_URL}" ]]; then
  echo "Error: no .tar.gz asset found in latest mpv-macos release." >&2
  exit 1
fi

ARCHIVE="$(mktemp -t mpv-macos.XXXXXX.tar.gz)"
echo "Downloading ${ASSET_URL} -> ${ARCHIVE}"
curl -fsSL -L -o "${ARCHIVE}" "${ASSET_URL}"

EXTRACT_DIR="$(mktemp -d -t mpv-extract.XXXXXX)"
tar -xzf "${ARCHIVE}" -C "${EXTRACT_DIR}"

# Find headers and dylib inside the extracted tree.
SRC_INCLUDE="$(find "${EXTRACT_DIR}" -type d -name mpv -path "*/include/*" | head -1)"
SRC_DYLIB="$(find "${EXTRACT_DIR}" -name "libmpv.dylib" -o -name "libmpv.2.dylib" | head -1)"

if [[ -z "${SRC_INCLUDE}" ]]; then
  echo "Error: mpv headers not found in archive." >&2
  exit 1
fi
if [[ -z "${SRC_DYLIB}" ]]; then
  echo "Error: libmpv.dylib not found in archive." >&2
  exit 1
fi

DST_INCLUDE="${BUNDLE_DIR}/include"
mkdir -p "${DST_INCLUDE}"
cp -R "${SRC_INCLUDE}" "${DST_INCLUDE}/"

DST_LIB="${BUNDLE_DIR}/lib"
mkdir -p "${DST_LIB}"
cp "${SRC_DYLIB}" "${DST_LIB}/libmpv.dylib"

echo "mpv bundle ready at ${BUNDLE_DIR}"
rm -rf "${ARCHIVE}" "${EXTRACT_DIR}"
