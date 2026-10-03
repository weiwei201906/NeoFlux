#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# NeoFlux - scripts/bootstrap.sh
#
# Initializes the git submodules NeoFlux needs under thirdparty/.
#
# A full recursive clone of every dependency is large and slow, so this script
# uses --depth 1 (shallow) by default. Pass --full to fetch complete history
# when you need to inspect or patch a dependency.
#
# Usage:
#   bash scripts/bootstrap.sh            # shallow, parallel
#   bash scripts/bootstrap.sh --full     # full history
#   bash scripts/bootstrap.sh --jobs 4   # limit parallel jobs
#
# Slow network? A mirror can be substituted per-submodule before init:
#   bash scripts/bootstrap.sh            # after setting a mirror below
# ---------------------------------------------------------------------------
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

DEPTH_ARGS=(--depth 1)
JOBS="$( (nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) )"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --full)      DEPTH_ARGS=() ; shift ;;
    --jobs)      JOBS="$2" ; shift 2 ;;
    -h|--help)
      sed -n '2,20p' "${BASH_SOURCE[0]}"
      exit 0 ;;
    *) echo "unknown option: $1" >&2 ; exit 2 ;;
  esac
done

cd "${REPO_ROOT}"

if [[ ! -f .gitmodules ]]; then
  echo "error: .gitmodules not found; run from the NeoFlux repository" >&2
  exit 1
fi

echo "Initializing submodules (${DEPTH_ARGS[*]:-full history}, jobs=${JOBS})..."
# --jobs only gained parallel support in Git 2.8+; fall back gracefully.
if git submodule update --init --recursive "${DEPTH_ARGS[@]}" \
     --jobs "${JOBS}" 2>/dev/null; then
  :
else
  echo "note: retrying without --jobs (older Git?)" >&2
  git submodule update --init --recursive "${DEPTH_ARGS[@]}"
fi

echo
echo "Submodules ready. Next:"
echo "  cmake -S . -B build -GNinja -DNEOFLUX_BUILD_TESTS=ON"
echo "  cmake --build build"
