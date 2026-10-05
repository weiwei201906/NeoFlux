# 平台原生调优层

平台原生调优层位于 `neoflux/src/native/`（命名空间 `neoflux::native`），是
框架中**唯一**允许直接调用操作系统底层 API 的地方——线程调度、定时器精度、
CPU 特性与缓存拓扑探测。这里的一切都是 **best-effort**：在受限系统上（权限
不足、内核特殊、CPU 不支持）静默降级为安全的 no-op 并记录一条日志，绝不抛异常、
绝不使调用方失败。

多数情况下你不需要直接调用它——渲染线程与事件循环已经替你接好了。本页说明每个
入口在各平台的行为、自动接入点，以及何时值得手动调用。

## 入口一览

| API | 用途 | 开销 | 何时调用 |
| --- | --- | --- | --- |
| `TuneRenderThread()` | 渲染线程的调度整形（提升优先级 / 向平台多媒体调度器注册）。 | 一次性，极低 | `RenderLoop` 顶部、首个 GPU 工作之前自动调用。 |
| `TuneUiThread()` | UI/事件循环线程的调度整形（定时器精度 + 优先级）。 | 一次性，极低 | `EventLoop::Run()` 开头自动调用。 |
| `PinThreadToBigCores()` | 在 big.LITTLE / 混合架构上把当前线程绑定到大核/性能核。 | 一次性 | `TuneRenderThread()` 之后自动调用。若你自建对 P 核敏感的工作线程则手动调用。 |
| `DetectCpuFeatures()` | SIMD 能力快照（`sse42`、`avx2`、`neon`、`neon_fp16`）。 | 极低，可重复 | 任意位置；结果仅供参考，调用方必须保留标量回退路径。 |
| `DetectCacheTopology()` | 运行时缓存几何（`CacheInfo`：一致性行大小 + L1d/L2/L3 字节数）。 | 低（一次性探测） | 启动时一次，或在为自建数据结构定尺寸之前。 |
| `PrefetchForRead(const void*)` | 单行读预取提示。 | 免费（单条指令） | 你自有的紧凑循环里，提前一次迭代。 |
| `PrefetchForWrite(const void*)` | 单行写预取提示（独占 / 为写准备）。 | 免费 | 向新领取的槽位或 slab 写入之前。 |
| `VerifyCacheLineConfig()` | 对比运行时行大小与编译期 `config::kCacheLineSize`；若硬件行更宽则**只告警一次**。 | 低 | 启动时一次（调试/遥测），在日志初始化之后。 |

::: tip 所有入口均为 `noexcept`
它们不会抛异常、不会中止进程。OS 调用失败只会变成一条日志并返回默认值——调用点
永远不会成为故障源。
:::

## 运行时配置（gflags）

所有调优尝试都可在运行时配置；flag 定义在 `neoflux/src/core/flags.cpp`，
文档见两份 README：

| Flag | 默认值 | 作用 |
|---|---|---|
| `--native_tuning` | `true` | 总开关：`false` 时以下所有入口均变为 no-op。 |
| `--native_render_rt_priority` | `1` | Linux/Android 渲染线程 SCHED_FIFO 优先级（1..99）。 |
| `--native_thread_nice` | `-5` | Linux/Android 渲染线程（降级路径）与 UI 线程的 nice 值。 |
| `--native_bigcore_threshold_permille` | `950` | 大核判定阈值（相对最快核频率的千分比，500..1000）。 |
| `--native_mmcss_profile` | `Games` | Windows MMCSS profile 名称。 |
| `--native_timer_period_ms` | `1` | Windows `timeBeginPeriod` 定时器精度（毫秒，`0` = 关闭）。 |

越界值会被钳制；OS 拒绝时一切尝试依旧静默降级——flag 只改变"多激进"，
绝不改变正确性。

## 平台行为矩阵

`✓` = 已实现；`no-op` = 在该平台上刻意不做任何事。

| 能力 | Windows | Linux | Android | macOS | iOS |
| --- | --- | --- | --- | --- | --- |
| `TuneRenderThread` | MMCSS "Games" 配置，否则 `ABOVE_NORMAL` | `SCHED_FIFO` 优先级 1，否则 nice −5 | `SCHED_FIFO`→nice，否则 no-op | QoS `USER_INTERACTIVE` | QoS `USER_INTERACTIVE` |
| `TuneUiThread` | 1 ms 定时器精度 + `ABOVE_NORMAL` 进程类别 | nice −5 | nice −5（通常被拒) | no-op（已是 `USER_INTERACTIVE`） | no-op |
| `PinThreadToBigCores` | 通过逻辑处理器拓扑的 `EfficiencyClass` | 对 ≥95% 最高频率的核调 `sched_setaffinity` | 同 Linux | no-op（由 QoS 决定集群） | no-op |
| `DetectCpuFeatures` | cpuid / xgetbv | cpuid / `getauxval` | `getauxval` | `sysctl`（Intel）/ ABI（Silicon） | ABI（Silicon） |
| `DetectCacheTopology` | cpuid leaf 1 + leaf 4 | sysfs 缓存树 | sysfs 缓存树 | `sysctl hw.*` | `sysctl hw.*` |
| `PrefetchForRead/Write` | `_mm_prefetch` | `__builtin_prefetch` | `__builtin_prefetch` | `__builtin_prefetch` | `__builtin_prefetch` |
| `VerifyCacheLineConfig` | ✓ | ✓ | ✓ | ✓ | ✓ |

全部基于 intrinsic 实现：本层任何地方都不使用手写内联汇编（cpuid 用
`__cpuid`/`__cpuidex`，预取用 `__builtin_prefetch` / `_mm_prefetch`）。

这句话只针对**本层**。框架确实带有少量手写 SIMD 内核——它们在下一级目录
`src/native/asm/` 里，通过 C ABI 接入，而不是从这里调用。参见
[手写汇编内核](./native-asm.md)。

## 自动接入点

正常路径下调优是免费获得的：

- **`RenderLoop`** —— 线程启动时调用一次 `TuneRenderThread()`，随后调用
  `PinThreadToBigCores()`，再进入帧循环。这个线程的调度延迟会直接转化为帧节拍
  抖动。
- **`EventLoop::Run()`** —— 开头调用一次 `TuneUiThread()`。在 Windows 上正是它
  把默认约 15.6 ms 的定时器粒度变为 1 ms；否则 60 FPS 的
  `condition_variable::wait_for()` 会出现肉眼可见的抖动。
- **启动 / 遥测** —— `DetectCpuFeatures()`、`DetectCacheTopology()` 与
  `VerifyCacheLineConfig()` 开销都很低，在 `google::InitGoogleLogging()` 之后
  调用一次即可。框架会以 `INFO` 记录检测到的拓扑，必要时抛出缓存行告警（见下）。

### 何时需要手动调用

| 场景 | 调用 |
| --- | --- |
| 你新建了一个做 GPU 相邻或延迟敏感工作的线程。 | `TuneRenderThread()`（+ `PinThreadToBigCores()`）。 |
| 想在 bug 报告里带一份缓存几何的 info 级转储。 | `DetectCacheTopology()` + `VerifyCacheLineConfig()`。 |
| 你自有无锁 / 缓存敏感的数据结构，想按真实行大小定填充。 | `DetectCacheTopology().line_size`。 |
| 你在自控的热循环里流式遍历一块缓冲区。 | 提前一次迭代调 `PrefetchForRead()` / `PrefetchForWrite()`。 |
| 目标是无桌面或非桌面平台，想确认实际生效了什么。 | 读 `native:` 开头的日志行——每条路径都会记录一次结果。 |

::: warning 不要逐帧调用调优函数
它们用于一次性的线程/进程整形。在循环里反复调 `PinThreadToBigCores()` 会与调度器
对抗，可能**降低**吞吐。在线程启动时调一次即可。
:::

## 缓存行定尺寸与编译期配置

`config::kCacheLineSize`（见 `neoflux/include/neoflux/core/config.h`）是用于把
SPSC 环形队列头/尾填充到不同缓存行的**编译期**对齐值。默认 64，可在配置期覆盖：

```bash
cmake -B build -DNEOFLUX_CACHE_LINE_SIZE=128
```

运行时的 `DetectCacheTopology()` / `VerifyCacheLineConfig()` 这一对，正是为了
捕捉编译期取值比硬件行**更窄**的情况。关键例子是 **Apple M 系列**：其 L1 数据
缓存一致性行为 **128 字节**；按 64 字节填充的队列可能让相邻两个对象落在同一行上，
于是生产者与消费者仍会在核间来回弹跳该行——伪共享并未因填充而消除。任何 Apple
Silicon 目标请这样构建：

```bash
cmake -B build -DNEOFLUX_CACHE_LINE_SIZE=128
```

`VerifyCacheLineConfig()` 比较 `CacheInfo::line_size` 与
`config::kCacheLineSize`，并且**至多记录一次**：

| 运行时 vs 编译期 | 日志 |
| --- | --- |
| 运行时 **>** 编译期 | `WARNING` —— 对齐不足、存在伪共享风险，并给出确切的 `-DNEOFLUX_CACHE_LINE_SIZE=` 修复方式。 |
| 运行时 **==** 编译期 | `INFO` —— 匹配。 |
| 运行时 **<** 编译期 | `INFO` —— 更保守（安全，代价是些许内存）。 |
| 探测不可用（无 sysfs、未知 CPU） | `INFO` —— 缓存行按 64 B 假定；不做比较。 |

完整的常量参考见 [配置](./configuration.md)。

## 验证实际发生了什么

每条路径恰好记录一次，前缀为 `native:`。一次健康的桌面运行大致如下：

```
I native: cache topology detected -- line=64B L1d=49152B L2=1310720B L3=56623104B
I native: compile-time cache line matches runtime (64B)
I native: render thread -> SCHED_FIFO prio=1
I native: ui thread -> nice -5
```

在 128 字节缓存行的机器上用 64 字节构建时，则会看到：

```
W native: runtime cache line (128B) exceeds compile-time
  config::kCacheLineSize (64B); SPSC queue head/tail may still share a
  coherence line -> false sharing. Rebuild with
  -DNEOFLUX_CACHE_LINE_SIZE=128 to fix it.
```

某条 `native:` 日志缺失绝不是错误——它意味着该平台路径是刻意的 no-op（例如
Apple 上的大核绑定，那里 QoS 已经驱动集群放置）。

## 另见

- [配置](./configuration.md) —— `config::kCacheLineSize` 及其他编译时常量。
- [调试与调优](./debugging.md) —— 日志参数与渲染队列背压问题。
- [跨平台](./cross-platform.md) —— 各平台翻译单元如何在配置期被选择。
- [手写汇编内核](./native-asm.md) —— `src/native/asm/` 下的汇编 SIMD 内核，
  以及它们为什么不用 intrinsic 写。

---

*自 NeoFlux 0.3.1-alpha + neoflux-fix 补丁集起提供。*
