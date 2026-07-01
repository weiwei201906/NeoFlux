#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
if [ ! -d build ]; then
  mkdir -p build
  cd build
  cmake ..
  cmake --build .
  cd ..
fi

./build/tests/neoflux_test
