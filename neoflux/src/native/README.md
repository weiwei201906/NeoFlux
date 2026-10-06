# src/native/ - platform-native tuning layer

**This directory is the only place in framework code allowed to call operating
system APIs directly.**

The Chinese version of this document is kept with the other translated
documentation as `docs/zh/guide/native-layer.md`.

## What belongs here

| Content | File |
|---|---|
| Interface declarations | `native_tuning.h` (namespace `neoflux::native`) |
| CPUID dispatch (the only file allowed to include `<intrin.h>`) | `cpuid_bits.h` |
| Shared x86 cache-topology walk | `cache_topology_cpuid.h` |
| Windows tuning | `windows/platform_win32.cpp` (thread priority, `timeBeginPeriod(1)`, big-core pinning) |
| Linux/Android tuning | `linux/platform_linux.cpp` (SCHED_FIFO with a nice fallback, cpufreq big-core pinning) |
| macOS/iOS tuning | `apple/platform_apple.cpp` (QoS class) and `apple/cache_topology_apple.cpp` (sysctl) |
| Fallback for other platforms | `platform_common.cpp` (everything is a no-op) |

Current capabilities: render-thread and UI-thread scheduling shaping, frame
pacing timer resolution, CPU SIMD feature detection (SSE4.2 / AVX2 / ASIMD /
FP16, including the OS-support check), cache-topology detection, and cache
prefetch hints.

## No compiler intrinsics on the hot paths

CPU instructions are reached through `asm/` rather than through a per-compiler
intrinsic, so the Windows, Linux and macOS code paths are the same source. The
consequences worth knowing:

- `cpuid_bits.h` is the only file that may include `<intrin.h>`, and only as a
  fallback for MSVC/clang-cl, which cannot assemble GAS syntax. Everywhere else
  the CPUID read is `neoflux_cpuid_subleaf` from `asm/cpuid_x86.S`.
- Cache topology has one implementation for x86 (`cache_topology_cpuid.h`) used
  by both Windows and Linux. Two copies of a bit-field decoder is two chances to
  transcribe a mask wrong, so there is now one.
- Prefetch hints are `neoflux_prefetch_read/write` from `asm/prefetch_x86.S`
  (PREFETCHT0) or `asm/prefetch_aarch64.S` (PRFM), never `__builtin_prefetch`
  or `_mm_prefetch`.
- Where a primitive is unavailable (an MSVC build, or an architecture with no
  such instruction) the C++ side degrades explicitly rather than guessing: an
  uninstrumented AVX2 read reports `avx2 = false`, and an unlinked prefetch is a
  no-op. Advertising an unverified capability is how a kernel corrupts a
  neighbouring thread.

## What does not belong here (lessons learned)

- **Any rendering or GPU API.** All GPU backends live behind tgfx, and the
  backend choice is tgfx's own compile-time `TGFX_USE_*` switch (see
  `thirdparty/CMakeLists.txt`). The `gl/gl_functions.{h,cpp}` pair that used to
  live here (a self-managed GL loader) was architecturally misplaced dead code
  and was deleted in `refactor_strip-gl-from-core`.
- **Media decoding or video interop.** That belongs to the media module
  (`neoflux/src/media/`).
- **Windowing.** That belongs to `src/renderers/glfw_bridge.cpp` or
  `mobile_bridge.cpp`.

## Conventions

- Every entry point is **best-effort**: when privileges are missing or the
  platform does not support the operation, it degrades silently and logs once.
  It never throws and never fails the caller.
- Adding a tuning capability means: define the cross-platform semantics in
  `native_tuning.h` first, then implement it in all four backends, including
  the fallback.
- CMake selects the implementation per platform (the "Platform-native tuning
  layer" block in `neoflux/CMakeLists.txt`); nothing is selected by hand.
