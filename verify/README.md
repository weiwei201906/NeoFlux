<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 NeoFlux Authors -->
# Standalone verification suite

Unit-level tests that compile **without CMake, tgfx, mpv or taitank** — they
exercise the framework's own sources directly. Useful as a fast pre-push check
and in CI environments that cannot pull the heavy third-party submodules.

```bash
# Dependencies (Ubuntu)
sudo apt-get install -y g++ libgoogle-glog-dev libgflags-dev

REPO=$(cd "$(dirname "$0")/.." && pwd)
V="$REPO/verify"
NATIVE_SRC=("$REPO/neoflux/src/core/flags.cpp"
            "$REPO/neoflux/src/native/linux/platform_linux.cpp"
            "$REPO/neoflux/src/native/linux/cache_topology_linux.cpp"
            "$REPO/neoflux/src/native/asm/xgetbv.S")   # Linux build
INC=(-I "$REPO/neoflux/include" -I "$REPO/neoflux/src")

# [1] Task coroutine semantics
g++ -std=c++20 "$V/test_task_await.cpp" "$V/yield_stub.cpp" -o /tmp/t1 && /tmp/t1

# [2] EventLoop: pacing / idle throttle / animation protection / coalescing
g++ -std=c++20 -O1 "${INC[@]}" "$V/test_event_loop.cpp" \
    "$REPO/neoflux/src/apps/event_loop.cpp" "${NATIVE_SRC[@]}" \
    -lglog -lgflags -lpthread -o /tmp/t2 && /tmp/t2

# [3] EventLoop reuse (no coroutine state leaks across Run()s; ASAN)
g++ -std=c++20 -O1 -fsanitize=address "${INC[@]}" "$V/test_event_loop_reuse.cpp" \
    "$REPO/neoflux/src/apps/event_loop.cpp" "${NATIVE_SRC[@]}" \
    -lglog -lgflags -lpthread -o /tmp/t3 && /tmp/t3

# [4] Ring queue: destructor destroys every element (ASAN+LSAN)
g++ -std=c++20 -O1 -fsanitize=address,leak "$V/verify_ring_queue_dtor.cpp" -o /tmp/t4 && /tmp/t4

# [5] Ring queue: non-default-constructible element types compile
g++ -std=c++20 -O1 "$V/verify_ring_queue_nodefault.cpp" -o /tmp/t5 && /tmp/t5

# [6] Centralized flags resolve (incl. --idle_fps and the --native_* group)
g++ -std=c++20 -O1 -I "$REPO/neoflux/include" "$V/verify_flags.cpp" \
    "$REPO/neoflux/src/core/flags.cpp" -lgflags -lglog -o /tmp/t6 && /tmp/t6

# [7] Compile-time backend macro injection
g++ -std=c++20 -O1 -DNEOFLUX_BACKEND_gl -DNEOFLUX_BACKEND_NAME='"gl"' \
    "$V/test_backend_macro.cpp" -o /tmp/t7 && /tmp/t7

# [8] Native tuning: smoke (4 cases, /proc/cpuinfo cross-check)
g++ -std=c++20 -O1 "${INC[@]}" "$V/test_native_smoke.cpp" "${NATIVE_SRC[@]}" \
    -lglog -lgflags -lpthread -o /tmp/t8 && /tmp/t8

# [9] Native tuning: death/degradation paths (ASAN)
g++ -std=c++20 -O1 -fsanitize=address "${INC[@]}" "$V/test_native_death.cpp" \
    "${NATIVE_SRC[@]}" -lglog -lgflags -lpthread -o /tmp/t9 && /tmp/t9

# [10] Native tuning: concurrent stress (TSAN, 8 threads x 5000 iters)
g++ -std=c++20 -O1 -fsanitize=thread "${INC[@]}" "$V/test_native_stress.cpp" \
    "${NATIVE_SRC[@]}" -lglog -lgflags -lpthread -o /tmp/t10 && /tmp/t10
```

All ten must pass (or print PASS) on any x86_64 Linux box. Windows/Apple
sources are syntax-checked with platform stub headers and are exercised by CI
on native runners.
