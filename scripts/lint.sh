#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
if [ ! -f build/compile_commands.json ]; then
  mkdir -p build
  cd build
  cmake ..
  cd ..
fi

clang-tidy \
  $(find framework app tests -name '*.cpp' -o -name '*.h' | tr '\n' ' ') \
  -- -p=build
