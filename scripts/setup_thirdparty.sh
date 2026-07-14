#!/usr/bin/env bash
# Initialize thirdparty git submodules for NeoFlux
# Author: weiwei201906
# Date: 2026-07-01
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

repos=(
  "https://github.com/gflags/gflags.git thirdparty/gflags main"
  "https://github.com/google/glog.git thirdparty/glog main"
  "https://github.com/google/googletest.git thirdparty/googletest main"
  "https://github.com/Tencent/Taitank.git thirdparty/taitank main"
)

for entry in "${repos[@]}"; do
  read -r url path ref <<<"$entry"
  if [ -e "$path" ] || git ls-files --error-unmatch "$path" >/dev/null 2>&1; then
    echo "Submodule path $path already exists, skipping"
    continue
  fi
  echo "Adding submodule $url -> $path (ref: $ref)"
  git submodule add "$url" "$path"
  if [ -n "$ref" ]; then
    git -C "$path" fetch --tags origin
    if git -C "$path" rev-parse --verify "refs/tags/$ref" >/dev/null 2>&1; then
      git -C "$path" checkout "refs/tags/$ref"
    elif git -C "$path" rev-parse --verify "origin/$ref" >/dev/null 2>&1; then
      git -C "$path" checkout "origin/$ref"
    else
      echo "Warning: ref '$ref' not found in $url, using default branch"
    fi
  fi
  git -C "$path" submodule update --init --recursive

done

git submodule update --init --recursive

echo "Thirdparty submodules initialized."
