#!/usr/bin/env bash
# =============================================================================
# sync_tgfx_vendor.sh
#
# Idempotent vendor sync for the pinned tgfx third_party dependencies listed in
# thirdparty/tgfx/DEPS. This is the bash equivalent of sync_tgfx_vendor.ps1.
#
# On CI, tgfx's nested submodules are already checked out via submodules: true;
# this script is only needed when working offline or behind a flaky network.
#
# Run from the repo root:  bash scripts/sync_tgfx_vendor.sh
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
TP_DIR="${REPO_ROOT}/thirdparty/tgfx/third_party"

# dir, owner/repo, pinned commit (from thirdparty/tgfx/DEPS)
vendors=(
  "vendor_tools|libpag/vendor_tools|fdcd3c28c02c9d39aeef5aefede96645b8bc593b"
  "pathkit|libpag/pathkit|6dc44ae85b4614d4e5accac2d7059d7420db9ed2"
  "skcms|libpag/skcms|0a4e501a0e61d62dc38a0157e4355a58f5b01b81"
  "zlib|madler/zlib|da607da739fa6047df13e66a2af6b8bec7c2a498"
  "libpng|glennrp/libpng|3061454d980de7d53608f594194cfac722721d2a"
  "libwebp|webmproject/libwebp|233960a0ad8c640acd458a6966dea09e12c1325a"
  "libjpeg-turbo|libjpeg-turbo/libjpeg-turbo|0a9b9721782d3a60a5c16c8c9a7abf3d4b1ecd42"
  "freetype|freetype/freetype|0a0221a1347e2f1e07c395263540026e9a0aa7c7"
  "harfbuzz|harfbuzz/harfbuzz|9ef44a2d67ac870c1f7f671f6dc98d08a2579865"
  "googletest|google/googletest|6910c9d9165801d8827d628cb72eb7ea9dd538c5"
  "json|nlohmann/json|fec56a1a16c6e1c1b1f4e116a20e79398282626c"
  "expat|libexpat/libexpat|92810461043fce37e70079b37ab1f04490a8f039"
  "concurrentqueue|cameron314/concurrentqueue|6dd38b8a1dbaa7863aa907045f32308a56a6ff5d"
  "highway|google/highway|a523516d35e22a4ba8e2e70a319062cb87352de6"
  "SPIRV-Cross|KhronosGroup/SPIRV-Cross|ebe2aa0cd80f5eb5cd8a605da604cacf72205f3b"
  "Vulkan-Headers|KhronosGroup/Vulkan-Headers|b5c8f996196ba4aa6d8f97e52b5d3b6e70f7e4e2"
  "volk|zeux/volk|0b17a763ba5643e32da1b2152f8140461b3b7345"
  "VulkanMemoryAllocator|GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator|1d8f600fd424278486eade7ed3e877c99f0846b1"
)

for entry in "${vendors[@]}"; do
  IFS='|' read -r dir repo commit <<< "$entry"
  dirPath="${TP_DIR}/${dir}"
  url="https://github.com/${repo}.git"

  head="$(git -C "${dirPath}" rev-parse HEAD 2>/dev/null || true)"
  if [[ "${head}" == "${commit}" ]]; then
    echo "[skip] ${dir} already at ${commit:0:10}"
    continue
  fi

  if [[ -d "${dirPath}" ]]; then
    echo "[re-clone] ${dir} (head=${head}) -> ${commit:0:10}"
    rm -rf "${dirPath}"
  else
    echo "[clone] ${dir}"
  fi

  ok=0
  for attempt in 1 2 3; do
    rm -rf "${dirPath}"
    git init -q "${dirPath}"
    git -C "${dirPath}" remote add origin "${url}"
    if git -C "${dirPath}" fetch -q --depth 1 origin "${commit}"; then
      if git -C "${dirPath}" checkout -q FETCH_HEAD; then
        ok=1
        break
      fi
    fi
    echo "  attempt ${attempt} failed; retrying"
  done
  if [[ "${ok}" -ne 1 ]]; then
    echo "FATAL: git clone failed for ${repo}" >&2
    exit 1
  fi
  echo "[ok]   ${dir} @ ${commit:0:10}"
done

# GL-only marker dirs so tgfx configure skips shaderc/glslang network sync.
for marker in shaderc glslang; do
  mPath="${TP_DIR}/${marker}"
  mkdir -p "${mPath}"
  if [[ ! -f "${mPath}/README.md" ]]; then
    cat > "${mPath}/README.md" <<EOF
# ${marker}
GL-only build marker: tgfx configure skips shaderc/glslang network sync.
shaderc/glslang are only required by the D3D12/Vulkan/Metal backends.
EOF
  fi
  echo "[marker] ${marker}"
done

echo "Vendor sync complete."
