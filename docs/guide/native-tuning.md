# Native Tuning Layer

The native tuning layer lives in `neoflux/src/native/` (namespace
`neoflux::native`) and is the **only** place in the framework allowed to talk to
OS-specific low-level APIs — thread scheduling, timer resolution, CPU feature
and cache-topology probing. Everything there is **best-effort**: on restricted
systems (no privileges, exotic kernels, unsupported CPUs) it degrades to a safe
no-op and logs once, never throwing and never failing the caller.

Most of the time you do not call it directly — the render and event loops
already do. This page explains what each entry point does per platform, where
the automatic hooks are, and when a manual call is worth it.

## The entry points

| API | Purpose | Cost | When to call |
| --- | --- | --- | --- |
| `TuneRenderThread()` | Scheduling shaping for the render thread (raises priority / registers with the platform multimedia scheduler). | One-time, cheap | Automatic at the top of `RenderLoop` before the first GPU work. |
| `TuneUiThread()` | Scheduling shaping for the UI/event-loop thread (timer resolution + priority). | One-time, cheap | Automatic at the start of `EventLoop::Run()`. |
| `PinThreadToBigCores()` | Pin the calling thread to "big"/performance cores on big.LITTLE / hybrid topologies. | One-time | Automatic after `TuneRenderThread()`. Manual if you spawn your own worker on a P-core–sensitive path. |
| `DetectCpuFeatures()` | Snapshot of SIMD capabilities (`sse42`, `avx2`, `neon`, `neon_fp16`). | Trivial, repeated | Anywhere; results are informational and callers must keep a scalar fallback. |
| `DetectCacheTopology()` | Runtime cache geometry (`CacheInfo`: coherence line size + L1d/L2/L3 bytes). | Cheap (one-time probe) | Once at startup, or before sizing your own data structures. |
| `PrefetchForRead(const void*)` | Architecture-specific read prefetch hint. | Free (a single instruction) | In tight loops you own, one iteration ahead. |
| `PrefetchForWrite(const void*)` | Architecture-specific write prefetch hint. | Free | Before a store into a newly claimed slot or slab. |
| `VerifyCacheLineConfig()` | Compares runtime line size against compile-time `config::kCacheLineSize`; logs a **WARNING once** if the hardware line is wider. | Cheap | Once at startup (debug/telemetry), after logging is initialised. |

::: tip All entry points are `noexcept`
None of them can throw or abort. A failed OS call becomes a log line and a
return to the default — you never need to guard a call site.
:::

## Runtime configuration (gflags)

Every tuning attempt is configurable at runtime; the flags are defined in
`neoflux/src/core/flags.cpp` and documented in the READMEs:

| Flag | Default | Effect |
|---|---|---|
| `--native_tuning` | `true` | Master switch: `false` turns every entry point below into a no-op. |
| `--native_render_rt_priority` | `1` | Linux/Android SCHED_FIFO priority for the render thread (1..99). |
| `--native_thread_nice` | `-5` | Linux/Android nice value for the render (fallback) and UI threads. |
| `--native_bigcore_threshold_permille` | `950` | Big-core frequency threshold in permille of the fastest core (500..1000). |
| `--native_mmcss_profile` | `Games` | Windows MMCSS profile name. |
| `--native_timer_period_ms` | `1` | Windows `timeBeginPeriod` resolution in ms (`0` = off). |

Out-of-range values are clamped, and every flag still degrades silently when
the OS refuses — the flags only change what is *attempted*, never correctness.

## Platform behaviour matrix

`✓` = implemented; `no-op` = deliberately does nothing on that platform.

| Capability | Windows | Linux | Android | macOS | iOS |
| --- | --- | --- | --- | --- | --- |
| `TuneRenderThread` | MMCSS "Games" profile, else `ABOVE_NORMAL` | `SCHED_FIFO` priority 1, else nice −5 | `SCHED_FIFO`→nice, else no-op | QoS `USER_INTERACTIVE` | QoS `USER_INTERACTIVE` |
| `TuneUiThread` | 1 ms timer resolution + `ABOVE_NORMAL` process class | nice −5 | nice −5 (usually denied) | no-op (already `USER_INTERACTIVE`) | no-op |
| `PinThreadToBigCores` | `EfficiencyClass` via logic-processor topology | `sched_setaffinity` on ≥95%-of-max-frequency cores | same as Linux | no-op (QoS drives clusters) | no-op |
| `DetectCpuFeatures` | cpuid / xgetbv | cpuid / `getauxval` | `getauxval` | `sysctl` (Intel) / ABI (Silicon) | ABI (Silicon) |
| `DetectCacheTopology` | cpuid leaf 1 + leaf 4 | sysfs cache tree | sysfs cache tree | `sysctl hw.*` | `sysctl hw.*` |
| `PrefetchForRead/Write` | no-op on MSVC | x86 or AArch64 assembly | x86 or AArch64 assembly | x86 or AArch64 assembly | x86 or AArch64 assembly |
| `VerifyCacheLineConfig` | ✓ | ✓ | ✓ | ✓ | ✓ |

The platform tuning layer is the only place allowed to call OS APIs directly.
CPU feature detection may use compiler intrinsics or platform ABI helpers, while
cache prefetch is provided by architecture-specific assembly in `src/native/asm/`
when the build enables it. On a platform without an assembler, both prefetch
functions deliberately become no-ops. Assembly is selected at compile time and
reached through the C ABI; it never leaks into business or render-pipeline
code. See [Hand-Written Assembly Kernels](./native-asm.md).

## Automatic integration points

You get the tuning for free in the normal path:

- **`RenderLoop`** — calls `TuneRenderThread()` once at thread start, then
  `PinThreadToBigCores()`, before entering the frame loop. This is the thread
  whose scheduling latency directly becomes frame-pacing jitter.
- **`EventLoop::Run()`** — calls `TuneUiThread()` once at the top. On Windows
  this is what turns the default ~15.6 ms timer granularity into 1 ms, without
  which a 60 FPS `condition_variable::wait_for()` visibly stutters.
- **Startup / telemetry** — `DetectCpuFeatures()`, `DetectCacheTopology()`, and
  `VerifyCacheLineConfig()` are cheap and safe to call once after
  `google::InitGoogleLogging()`. The framework logs the detected topology at
  `INFO` and raises the cache-line warning (see below) if needed.

### When to call manually

| Situation | Call |
| --- | --- |
| You create a thread that does GPU-adjacent or latency-critical work. | `TuneRenderThread()` (+ `PinThreadToBigCores()`). |
| You want an info-level dump of the cache geometry in your own bug report. | `DetectCacheTopology()` + `VerifyCacheLineConfig()`. |
| You own a lock-free / cache-sensitive data structure and want to size padding from the real line size. | `DetectCacheTopology().line_size`. |
| You are streaming through a buffer in a hot loop you control. | `PrefetchForRead()` / `PrefetchForWrite()` one iteration ahead. |
| You target bare-metal or a non-desktop platform and want to confirm what actually took effect. | Read the `native:` log lines — every path logs its outcome once. |

::: warning Do not call the tuning functions per frame
They are for one-time thread/process shaping. Calling `PinThreadToBigCores()`
in a loop fights the scheduler and can *hurt* throughput. Call once, at thread
startup.
:::

## Cache-line sizing and the compile-time config

`config::kCacheLineSize` (see `neoflux/include/neoflux/core/config.h`) is the
**compile-time** alignment used to pad the SPSC ring queue head/tail into
separate cache lines. It defaults to 64 and can be overridden at configure time:

```bash
cmake -B build -DNEOFLUX_CACHE_LINE_SIZE=128
```

The runtime `DetectCacheTopology()` / `VerifyCacheLineConfig()` pair exists to
catch the case where the compile-time value is *narrower* than the hardware
line. The critical example is the **Apple M series**, whose L1 data cache
coherence line is **128 bytes**: a queue padded to 64 bytes can land two
adjacent objects on one line, so the producer and consumer still bounce that
line between cores — false sharing survives the padding. On any Apple Silicon
target, build with:

```bash
cmake -B build -DNEOFLUX_CACHE_LINE_SIZE=128
```

`VerifyCacheLineConfig()` compares `CacheInfo::line_size` with
`config::kCacheLineSize` and logs **at most once**:

| Runtime vs compile-time | Logged |
| --- | --- |
| runtime **>** compile-time | `WARNING` — alignment insufficient, false-sharing risk, with the exact `-DNEOFLUX_CACHE_LINE_SIZE=` fix. |
| runtime **==** compile-time | `INFO` — matched. |
| runtime **<** compile-time | `INFO` — conservative (safe, slight memory cost). |
| probe unavailable (no sysfs, unknown CPU) | `INFO` — cache line assumed 64 B; no comparison. |

See [Configuration](./configuration.md) for the full constant reference.

## Verifying what happened

Every path logs exactly once, prefixed `native:`. A healthy desktop run looks
like:

```
I native: cache topology detected -- line=64B L1d=49152B L2=1310720B L3=56623104B
I native: compile-time cache line matches runtime (64B)
I native: render thread -> SCHED_FIFO prio=1
I native: ui thread -> nice -5
```

On a 128-byte-line machine with a 64-byte build you would instead see:

```
W native: runtime cache line (128B) exceeds compile-time
  config::kCacheLineSize (64B); SPSC queue head/tail may still share a
  coherence line -> false sharing. Rebuild with
  -DNEOFLUX_CACHE_LINE_SIZE=128 to fix it.
```

Absence of a `native:` line is never an error — it means the platform path was
a deliberate no-op (for example, core pinning on Apple, where QoS already
drives cluster placement).

## See also

- [Configuration](./configuration.md) — `config::kCacheLineSize` and the other
  compile-time constants.
- [Debugging & Tuning](./debugging.md) — logging flags and the render-queue
  back-pressure story.
- [Cross-Platform](./cross-platform.md) — how the per-platform translation
  units are selected at configure time.
- [Hand-Written Assembly Kernels](./native-asm.md) -- the assembly SIMD kernels
  in `src/native/asm/`, and why they are not written with intrinsics.

---

*Available since NeoFlux 0.3.1-alpha + the neoflux-fix patch set.*
