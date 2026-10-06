# src/native/ - platform-native tuning layer

**This directory is the only place in framework code allowed to call operating
system APIs directly.**

The Chinese version of this document is kept with the other translated
documentation as `docs/zh/guide/native-layer.md`.

## What belongs here

| Content | File |
|---|---|
| Interface declarations | `native_tuning.h` (namespace `neoflux::native`) |
| Windows tuning | `windows/platform_win32.cpp` (thread priority, `timeBeginPeriod(1)`, cpuid/xgetbv) |
| Linux/Android tuning | `linux/platform_linux.cpp` (SCHED_FIFO with a nice fallback, cpuid/getauxval) |
| macOS/iOS tuning | `apple/platform_apple.cpp` (QoS class, sysctl) |
| Fallback for other platforms | `platform_common.cpp` (everything is a no-op) |

Current capabilities: render-thread and UI-thread scheduling shaping, frame
pacing timer resolution, and CPU SIMD feature detection (SSE4.2 / AVX2 / ASIMD
/ FP16, including the OS-support check).

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
