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
#   bash scripts/bootstrap.sh --retries 5
#
# Slow network? A mirror can be substituted per-submodule before init:
#   bash scripts/bootstrap.sh            # after setting a mirror below
#
# Why the extra repair and retry logic exists
# -------------------------------------------
# CI caches the whole of thirdparty/ (see the "Cache submodules" step in
# .github/workflows/ci.yml). A cache entry can be restored into a checkout
# where it does not belong, and then every submodule working tree carries a
# .git FILE whose gitdir points at the cache path recorded when the entry was
# created. `git submodule update` refuses to touch such a directory and exits
# 128, so the job dies before it ever configures the build.
#
# The same run can fail that step in one matrix job and pass it in another,
# because the cache is per runner OS and the restore race is not deterministic.
# Therefore: repair the pointers first, retry the update a few times, and make
# every retry visible in the log.
# ---------------------------------------------------------------------------
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

DEPTH_ARGS=(--depth 1)
JOBS="$( (nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) )"
RETRIES=4
CLEANED=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --full)      DEPTH_ARGS=() ; shift ;;
    --jobs)      JOBS="$2" ; shift 2 ;;
    --retries)   RETRIES="$2" ; shift 2 ;;
    -h|--help)
      sed -n '2,32p' "${BASH_SOURCE[0]}"
      exit 0 ;;
    *) echo "unknown option: $1" >&2 ; exit 2 ;;
  esac
done

cd "${REPO_ROOT}"

if [[ ! -f .gitmodules ]]; then
  echo "error: .gitmodules not found; run from the NeoFlux repository" >&2
  exit 1
fi

# Read one key from a [submodule "..."] section of .gitmodules.
# The pristine file has no tabs in it, so a tab refuses the match and keeps
# key lines (path/url) from being mistaken for section headers.
submodule_path() {
  sed -n "s/^[^#]*path = \(.*\)$/\1/p" .gitmodules | sed -n "${1}p"
}

submodule_count() {
  sed -n 's/^\[submodule .*\]$/[x]/p' .gitmodules | wc -l | tr -d ' '
}

# A restored cache can leave a submodule path in a state git refuses to touch.
# Two shapes are handled, and both make `git submodule update --init` fail with
#   fatal: destination path '...' already exists and is not an empty directory
# or with a gitdir that belongs to another checkout:
#
#   1. the path holds a working tree but no .git of its own -- the cache was
#      restored without the gitdir the submodule needs, so git sees a plain
#      directory where it wants to clone;
#   2. the path holds a .git FILE pointing at a gitdir outside this repository
#      (the path recorded when the cache entry was created).
#
# Deleting either is safe: the content is a dependency checkout that
# `git submodule update --init` immediately re-populates from the pinned
# revision. Nothing in this repository is tracked inside thirdparty/.
repair_submodule_pointers() {
  local total index path gitfile recorded
  total="$(submodule_count)"
  for (( index = 1; index <= total; index++ )); do
    path="$(submodule_path "${index}")"
    [[ -n "${path}" ]] || continue
    [[ -e "${path}" ]] || continue

    if [[ ! -e "${path}/.git" ]]; then
      echo "note: ${path} is not a git checkout (stale cache entry);" >&2
      echo "      removing it so git can clone the pinned revision" >&2
      rm -rf -- "${path}"
      CLEANED=$((CLEANED + 1))
      continue
    fi

    [[ -f "${path}/.git" ]] || continue
    gitfile="$(cat "${path}/.git" 2>/dev/null || true)"
    recorded="${gitfile#gitdir: }"
    [[ "${recorded}" == "${gitfile}" ]] && continue
    case "${recorded}" in
      "${REPO_ROOT}"/*) ;;
      *)
        echo "note: ${path} points at a gitdir outside this checkout" >&2
        echo "      (${recorded}); removing it so the cache cannot pin it" >&2
        rm -rf -- "${path}"
        CLEANED=$((CLEANED + 1))
        ;;
    esac
  done
}

# `--jobs` only gained parallel support in Git 2.8+; fall back gracefully.
#
# errexit is disabled for the duration of the git calls on purpose. The caller
# must see their exit status to decide whether to retry, and with `set -e` a
# failing command inside a function terminates the whole script before the
# retry loop ever runs. This is the one place in this script where a nonzero
# status is an expected outcome rather than an error.
update_submodules() {
  local rc=0
  set +e
  git submodule update --init --recursive "${DEPTH_ARGS[@]}" \
    --jobs "${JOBS}"
  rc=$?
  if (( rc != 0 )); then
    echo "note: parallel update failed (git exit ${rc}); retrying without --jobs" >&2
    git submodule update --init --recursive "${DEPTH_ARGS[@]}"
    rc=$?
  fi
  set -e
  return "${rc}"
}

repair_submodule_pointers
echo "Initializing submodules (${DEPTH_ARGS[*]:-full history}, jobs=${JOBS})..."

succeeded=0
for (( attempt = 1; attempt <= RETRIES; attempt++ )); do
  if update_submodules; then
    succeeded=1
    break
  fi
  echo "note: submodule update failed (attempt ${attempt}/${RETRIES})" >&2
  if (( attempt < RETRIES )); then
    repair_submodule_pointers
    sleep "$(( attempt * 5 ))"
  fi
done

if (( succeeded == 0 )); then
  echo "error: submodule init failed after ${RETRIES} attempts" >&2
  exit 1
fi

echo "note: repaired ${CLEANED} stale submodule pointer(s)"
echo
echo "Submodules ready. Next:"
echo "  cmake -S . -B build -GNinja -DNEOFLUX_BUILD_TESTS=ON"
echo "  cmake --build build"
