#!/usr/bin/env bash
set -euo pipefail

mkdir -p "$(dirname "$0")/../build"
cd "$(dirname "$0")/../build"
cmake ..
cmake --build .
