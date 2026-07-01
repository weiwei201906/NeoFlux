#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
find framework app tests -name '*.[ch]pp' -o -name '*.[ch]' | xargs clang-format -i
