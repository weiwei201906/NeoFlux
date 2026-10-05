# 手写汇编内核

NeoFlux 把少量性能关键的内核以手写汇编的形式放在
`neoflux/src/native/asm/`，通过**纯 C ABI**调用，绝不在 C++ 源码里写内嵌 asm。
本页说明这些内核为什么存在、树的哪些层允许碰 SIMD、每个内核必须遵守的契约，
以及新增一个内核的清单。

多数贡献者不需要读这一页。内核对外的唯一入口是
`neoflux::native::PremultiplyRgba8()`：它是可移植的，在所有平台都有标量回退，
框架其余部分调用的就是它。

## 为什么是汇编而不是 intrinsic

intrinsic 是一次**请求**，不是一条指令。编译器自行决定指令序列、寄存器分配
和调度，而且每个编译器决定得都不一样。对一个必须在 GCC / Clang / MSVC /
AppleClang 上逐字节一致的内核来说，这种自由度本身就是 bug：

| | Intrinsic | 手写汇编 |
| --- | --- | --- |
| 指令选择 | 各编译器自行决定 | 由源文件固定 |
| 寄存器分配 | 各编译器自行决定 | 由源文件固定 |
| 跨编译器输出 | 相近，但不保证一致 | 构造上即逐字节一致 |
| 换新工具链 | 可能静默改变代码生成 | 同样的代码，或明确的链接错误 |
| 读懂内核 | 需要了解编译器行为 | 读一个文件即可 |

因此，当内核的 **ABI 行为**或**数值结果**不能依赖编译器的选择时，汇编才是
正确的工具。反过来，如果编译器已有稳定 intrinsic、且结果不依赖具体指令选择，
那就该用 intrinsic 并落到 platform 层（见
[什么时候不该写汇编](#什么时候不该写汇编)）。

::: tip 汇编不是为了走性能捷径
premultiply 内核写成汇编的理由是**确定性**，不是速度。编译器很乐意帮你把标量
循环向量化；它不会承诺的是：明天、或者换个编译器，仍然选出同样的指令和同样的
寄存器。
:::

## 目录与分层策略

SIMD 不允许泄漏到调用它的层。业务代码、渲染管线、调度层调用
`PremultiplyRgba8()`，它们**不需要**知道 SSE2 或 NEON 的存在。

| 目录 | Intrinsic 头 | 内嵌 asm | 调用 C ABI 内核 |
| --- | --- | --- | --- |
| `src/apps/`、`src/widgets/`、`src/core/`、业务代码 | 禁止 | 禁止 | 否（用 `PremultiplyRgba8()`） |
| 渲染管线、调度层 | 禁止 | 禁止 | 否（用 `PremultiplyRgba8()`） |
| `src/native/`（dispatch / 特性探测 / platform） | 允许 | 禁止 | 是，它就是 dispatch 层 |
| `src/native/asm/` | 不适用（它本身就是汇编） | 唯一允许放 `.S` 的地方 | 不适用 |
| `verify/`、`tests/` | 允许，且不进发布产物 | 禁止 | 是，仅用于对照校验 |

这些规则由 CI 的 `policy` job 强制执行：

- **C++ 中禁止内嵌 asm** —— 所有 `.h`/`.cpp` 中出现 `__asm__`、`__asm`、
  `asm volatile` 即失败。手写例程一律放在 `src/native/asm/`，通过
  `extern "C"` 声明接入。这正是内核得以独立于任何编译器的内嵌 asm 模型的原因。
- **`src/native/`、`tests/`、`verify/` 之外禁止 intrinsic 头** ——
  `<immintrin.h>`、`<emmintrin.h>`、`<arm_neon.h>`、`<intrin.h>` 等一律拒绝。
  匹配前会先剥掉行注释，因为规则本身就写在必然要拼出这些被禁 token 的注释里。
- **汇编 ABI 契约** —— 每个 `.S` 必须导出一个 `asm_symbols.h` 确实声明过的
  `neoflux_` 前缀符号，并遵守下面的寄存器纪律。

::: warning 测试里用 intrinsic 是刻意的
`verify/test_native_simd.cpp` 会包含 `<emmintrin.h>`。如果只拿 dispatch 层自己
的标量内核做对照，那么共享算术里的 bug 会自己骗过自己，所以测试还要比对
**第二个独立的 intrinsic oracle**。该文件只编译成一次性二进制，从不发布。
:::

### 什么时候不该写汇编

如果某个操作编译器已有稳定 intrinsic、且结果不依赖具体指令选择，就不要写汇编：
用 intrinsic 实现，放在 platform 层（`src/native/`），不要扩散出去。仓库里就有
三个现成的例子：

- **cpuid** —— `__cpuid` / `__cpuidex`。
- **预取** —— `__builtin_prefetch` / `_mm_prefetch`。
- **MSVC 上的 xgetbv** —— `_xgetbv` intrinsic；正因如此，`asm/xgetbv.S` 只在
  非 MSVC 的 x86-64 构建里被链接。

## 已有内核

| 符号 | 文件 | 指令集 | 每次像素数 | 架构基线？ |
| --- | --- | --- | --- | --- |
| `neoflux_premultiply_rgba8` | `asm/premultiply_rgba_x86_64.S` | SSE2 | 4 | 是（x86-64） |
| `neoflux_premultiply_rgba8` | `asm/premultiply_rgba_aarch64.S` | NEON / ASIMD | 8 | 是（AArch64） |
| `neoflux_read_xcr0` | `asm/xgetbv.S` | x86-64（非 SIMD） | 不适用 | 不适用 |

两个 premultiply 文件定义的是**同一个**符号名，这是刻意的：CMake 对每个目标
只链接其中一个，于是 `src/native/simd_kernels.cpp` 里的 dispatch 层可以完全
不知道架构，也永远不必说出某个指令集的名字。

### 为什么没有运行时 CPU 探测

`src/native/native_tuning.h` 里的 `DetectCpuFeatures()` 会报告 `sse42`、
`avx2`、`neon`、`neon_fp16`。一个需要 AVX2 或 SVE2 的内核必须在运行时查它，
因为那些是可选扩展，而且操作系统也可能拒绝使能（x86 上是 XCR0，也就是
`asm/xgetbv.S` 读的那个；AArch64 上是 `HWCAP`）。

已发布的两个内核不同：**SSE2 是 x86-64 的架构基线**，**Advanced SIMD（NEON）
是 AArch64 的架构基线**——这些目标的每一种实现都保证它们。运行时探测没有任何
可决定的东西，只会给热路径多加一个分支，所以选择纯粹发生在编译期，按目标架构
决定。

## 内核契约

`src/native/asm/` 下每个内核都遵守同一份契约：

1. **固定的 N** —— SSE2 下 4 个像素，NEON 下 8 个，并在文件头注明。
2. **返回已转换的像素数** —— `floor(pixels / N) * N`。
3. **绝不碰尾部** —— 超出返回数量的像素完全不动，调用方可以用标量内核补齐，
   不会有任何像素被转换两次。
4. **允许别名** —— `dst` 可以等于 `src`。
5. **没有错误分支** —— 空指针与零长度的守卫属于 dispatch 封装
   （`PremultiplyRgba8()`），不属于汇编。

公式（通道 `c` 取 `{R, G, B}`）：

```
dst[c] = (src[c] * src[a] + 127) / 255
dst[a] = src[a]
```

### 除以 255 是精确的

这里的除法不是移位近似。令 `t = channel * alpha + 127`：

```
(t + (t >> 8)) >> 8   ==   floor(t / 255)   对 [0, 65152] 内所有 t 成立
```

`channel * alpha` 最大 65025，所以 `t` 最大 65152，且所有中间值都不超过
65406——16 位通道永不溢出，不需要扩宽到 32 位。不要把它"简化"成只右移 8 位：
那会对每一个被舍入项推过边界的值都差一。

## ABI 与寄存器纪律

这是最容易踩、也最难发现的坑。一个破坏了被调用方保存寄存器的内核，在那个寄存器
恰好是调用方保存的 ABI 上能通过全部测试，而在另一个 ABI 上静默毁掉调用方。

| 目标 | 被调用方保存的向量寄存器 | 内核可用的寄存器 |
| --- | --- | --- |
| x86-64（SysV AMD64 与 Win64） | Win64：`xmm6`-`xmm15`；SysV：无 | 只能用 `xmm0`-`xmm5` |
| AArch64（AAPCS64） | `v8`-`v15` 的低 64 位 | `v0`-`v7` 与 `v16`-`v31` |

两个 64 位 x86 ABI 的交集是 `xmm0`-`xmm5`，所以 x86-64 内核把自己严格限制在
这六个之内。碰 `xmm6` 及以上，正是"手写内核在 Linux 上通过、在 Windows 上悄悄
毁掉调用方状态"的经典成因。

已发布的 AArch64 内核用了 `v0`-`v7` 加 `v16`、`v17`。CI 会拒绝任何
`*_aarch64.S` 中出现 `v8`-`v15`。

内核还依赖以下 ABI 事实：

- **整型参数**在两个 64 位 x86 ABI 上不同（SysV 用 `RDI`/`RSI`/`RDX`，Win64 用
  `RCX`/`RDX`/`R8`）。用宏隐藏差异，一份指令流服务两者。
- **只用调用方保存的通用寄存器** —— x86-64 内核用 `R10` 做循环计数、`RAX` 存
  返回值；AArch64 内核用 `X3` 和 `X9`。没有任何东西需要保存与恢复。
- **Apple 符号修饰** —— Mach-O 会给 C 符号加前导下划线，用 `SYMBOL_NAME()`
  宏隐藏。这也是这些文件使用大写 `.S`（走 C 预处理器）的原因。
- **不建 `.rodata`** —— 常量用 ALU 指令现场构造（`pcmpeqd` / `psrlw` /
  `psllq` / `movi`），保持位置无关。

## 如何新增一个内核

按顺序走完这份清单。第 2 至 6 步各有一道 CI 门禁，所以漏掉任何一步都是一次
红色构建，而不是一次静默的遗漏。

1. **先判断该不该写成汇编。** 如果已有稳定 intrinsic、且结果不依赖指令选择，
   就此打住，改用 intrinsic 放到 platform 层。
2. **在 `neoflux/src/native/asm/` 写 `.S`** —— 一个文件一个函数，符号带
   `neoflux_` 前缀，文件头写清契约与所遵循的 ABI，每条指令后都有注释，并遵守
   上面的寄存器纪律。循环沿用现有内核的形状：固定组大小、返回数量、绝不碰尾部。
3. **在 `asm/asm_symbols.h` 里登记** `extern "C"` 声明，用
   `NEOFLUX_NATIVE_ASM_*` 宏保护。不要在调用点自己写 `extern "C"`。CI 会检查
   每个 `.S` 都导出了本头声明过的符号，所以未登记的文件不可能"悄悄从未被链接"。
4. **在 `neoflux/CMakeLists.txt` 里按目标架构选用** —— 显式
   `target_sources(...)` 加对应的私有 `target_compile_definitions(...)`，没有
   glob。MSVC 除外：它汇编的是 MASM（`.asm`）而不是 GAS（`.S`），发布一份这里
   无法验证的 MASM 翻译比直接回退更糟。
5. **在 `src/native/simd_kernels.{h,cpp}` 里接上 dispatch** —— 新增
   `SimdLevel` 枚举项、组大小、编译期分派，以及标量尾部路径。对 C ABI 符号的
   调用要用 `#if defined(...)` 保护，**不要**用 `if (group != 0)`：常量为假的
   运行时条件仍然会留下一个链接期引用，而没有哪个编译器承诺会把它擦掉。
6. **若指令集不是架构基线**，用 `DetectCpuFeatures()`（以及 OS 可能拒绝时的
   XCR0 / HWCAP）做运行时门禁，并保留可达的标量路径。已发布的两个内核不需要
   这些。
7. **给 `verify/test_native_simd.cpp` 加对照用例**，并把编译命令补进
   `verify/README.md`。
8. **更新文档** —— 本页，以及（如果内核对用户可见）相应的指南页。

::: warning 两个独立开关就是两次不一致的机会
CMake 为每个内核只定义一个架构宏，`NEOFLUX_NATIVE_ASM_PREMULTIPLY` 这个总开关
是在 `asm_symbols.h` 里推导出来的。分开定义两者会让它们有机会互相矛盾，而这里
的矛盾表现为链接错误，不是回退。
:::

## 构建配置

| 配置 | 生效的内核 | `ActiveSimdLevel()` |
| --- | --- | --- |
| x86-64 上的 GCC / Clang / AppleClang | `premultiply_rgba_x86_64.S` | `kSse2` |
| AArch64 上的 GCC / Clang / AppleClang | `premultiply_rgba_aarch64.S` | `kNeon` |
| MSVC（任何架构） | 无，纯标量 | `kScalar` |
| 其他处理器 | 无，纯标量 | `kScalar` |

`simd_kernels.cpp` 里的标量内核**永远**被编译进来：它既是尾部补齐者，也是没有
汇编被链接时的完整实现。没有内核的构建是受支持的配置，不是坏掉的构建——CMake
配置阶段会打印它选了哪条路径。

## 如何验证

`verify/test_native_simd.cpp` 是回归套件，不需要 CMake、tgfx、mpv 或 taitank
即可编译。摘自 `verify/README.md`：

```bash
SIMD_ASM="$REPO/neoflux/src/native/asm/premultiply_rgba_x86_64.S"
g++ -std=c++20 -O1 -DNEOFLUX_NATIVE_ASM_PREMULTIPLY_X86_64=1 "${INC[@]}" \
    "$V/test_native_simd.cpp" \
    "$REPO/neoflux/src/native/simd_kernels.cpp" "$SIMD_ASM" \
    -o /tmp/t11 && /tmp/t11
```

arm64 上改用 `-DNEOFLUX_NATIVE_ASM_PREMULTIPLY_AARCH64=1` 与
`premultiply_rgba_aarch64.S`。两者都省略时套件仍然能跑：汇编用例报 `SKIP`，
标量路径被完整覆盖。

| 用例 | 类别 | 证明什么 |
| --- | --- | --- |
| 1 | smoke | 手挑像素与两种退化 alpha（`0` 把 RGB 清成零，`255` 必须精确恒等） |
| 2 | smoke | 尾部长度 `0..40` 全覆盖，外加 alpha 0 与 alpha 255 扫描 |
| 3 | stress | 65536 个随机像素（256 KiB），每个缓冲区后面都带哨兵守卫字节 |
| 4 | death | 恶意参数（空指针、`SIZE_MAX`、短于一个组的数量）绝不崩溃；POSIX 上在 fork 出的子进程中跑 |
| 5 | smoke | 原地 `dst == src` 与非原地结果一致 |
| 6 | oracle | 裸 C ABI 内核与标量参考一致，且不写超出返回数量的任何字节 |
| 7 | oracle | 汇编与第二个用 SSE2 intrinsic 写的 oracle 一致 |

每个缓冲区都带哨兵带，所以多走一个组的内核会响亮地失败，而不是悄悄越界涂写。

## 另见

- [平台原生调优层](./native-tuning.md) —— 这些内核外圈的 platform 层：
  `DetectCpuFeatures()`、缓存拓扑、线程调度。
- [跨平台](./cross-platform.md) —— 各平台翻译单元（以及这些 `.S`）在配置期如何
  被选择。
- [测试](./testing.md) —— 上面用到的 smoke / death / stress 三类。
- `neoflux/src/native/asm/README.md` —— 面向贡献者的版本，含易读性要求与符号
  命名约定。

---

*自第一个手写 SIMD 内核（SSE2 / NEON premultiply）起提供。*
