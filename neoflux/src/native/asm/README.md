# src/native/asm/ — 手写汇编例程

**本目录存放少量性能关键、且无法用 C++ 或编译器 intrinsic 表达的平台汇编例程。**

## 放什么

- **单一职责的独立函数**：一个 `.S` 文件实现**一个**函数，命名如下（见下）。
- **性能关键的裸指令序列**：例如需要精确控制指令选择、寄存器使用或
  越过编译器抽象边界的场景。
- **无法用 intrinsic 表达的指令**：当目标平台/编译器没有对应内置函数时，
  手写一小段汇编是允许的（`xgetbv.S` 即此例）。

## 不放什么

- **内嵌 asm**：C++ 源码中**禁止**出现 `__asm__` / `__asm`（项目硬性规则）。
  平台相关汇编一律外置到本目录。
- **复杂逻辑**：循环、分支、数据结构操作用 C++/intrinsic 实现，不要写进
  `.S`。本目录只应剩下"几条指令就能说清楚"的东西。**唯一例外是 SIMD 内核，
  见下节**——`premultiply_rgba_*.S` 就带循环。
- **业务代码**：渲染、媒体、窗口、调优策略等业务逻辑属于各自模块，
  不得进入本目录。
- **编译器 intrinsic 已经覆盖的能力**：若有等价 intrinsic（如 MSVC `_xgetbv`、
  `__cpuid`），优先用 intrinsic——它不算内嵌 asm，且更可移植。

## 唯一例外：SIMD 内核（允许循环）

"不放复杂逻辑"指的是**不要用汇编写业务逻辑**。有一类代码不在此列：

**性能关键、且需要指令确定性 / 精确寄存器控制 / 跨编译器一致性的 SIMD 内核。**

判据是：**如果用 intrinsic 写，不同编译器会选出不同指令、做出不同寄存器
分配，而内核的ABI 行为或数值结果不能依赖这种差异**——那么它就该是手写汇编。
`premultiply_rgba_x86_64.S`（SSE2）与 `premultiply_rgba_aarch64.S`（NEON）即此类：
同一公式在 GCC / Clang / MSVC / AppleClang 上必须逐字节一致，且必须只使用
调用方不需要保存的寄存器。

这类文件除遵守上面的通用要求外，**还必须**满足：

- [ ] **一次处理 N 个像素**是固定的（x86-64 为 4，AArch64 为 8），并在文件头
      注明；**返回已处理的数量**，尾部像素一律不碰。
- [ ] 尾部由 `src/native/simd_kernels.cpp` 的可移植标量内核补齐——汇编永远
      不必处理"最后几个"。
- [ ] **只使用调用方无需保存的寄存器**。这是最容易踩的坑：
      Win64 的 `xmm6`-`xmm15` 是被调用方保存的（SysV 则没有），所以 x86-64
      内核**只用 `xmm0`-`xmm5`**；AAPCS64 的 `v8`-`v15` 低 64 位是被调用方
      保存的，所以 AArch64 内核**只用 `v0`-`v7` 和 `v16`-`v17`**。
      破坏这条的代码在 Linux 上通过、在 Windows 上悄悄毁掉调用方寄存器。
- [ ] 双 ABI（SysV AMD64 / Win64）用宏区分参数寄存器，一份指令流服务两者。
- [ ] 常量用 ALU 指令现场构造（`pcmpeqd` / `psrlw` / `psllq` / `movi`），
      不建 `.rodata`——保持位置无关。
- [ ] 在 `asm_symbols.h` 登记 C ABI 声明（**不要**在调用点自己写
      `extern "C"`），并由 CMake 只在选定该 `.S` 时定义对应宏。
- [ ] 有对照测试：`verify/test_native_simd.cpp` 同时比对了**标量参考**与
      **SSE2 intrinsic 参考**，两者都要逐字节一致。

> 反过来：如果某个操作编译器已有稳定 intrinsic、且结果不依赖具体指令选择，
> 就**不要**写汇编——那属于下面"不放什么"的第四点，应落到 platform 层
> （`src/native/`）用 intrinsic 实现，不扩散到业务代码。

## 易读性要求（逐条对照）

- [ ] 文件头注释包含：SPDX/版权、函数契约（签名 + 语义 + 返回值）、
      所遵循的 ABI（SysV AMD64 / Win64）及关键 ABI 说明。
- [ ] **每条指令**后面有行内注释，说明该指令在做什么。
- [ ] 单一函数职责——一个文件一个函数。
- [ ] 符号使用 `neoflux_` 前缀，避免链接期与其他库冲突。
- [ ] 说明平台/位宽约束（例如"仅 64 位，i386 不在编译矩阵"）。
- [ ] 如涉及特殊处理（如 Apple 前导下划线），有显式注释解释。

## 符号命名约定

- 所有对外符号统一 `neoflux_` 前缀，全部为 C 链接（`extern "C"`）。
- C++ 侧声明集中放在 [`asm_symbols.h`](asm_symbols.h)，**不要**在调用点直接
  写 `extern "C"` 声明。
- **Apple 特例**：Mach-O 会给 C 符号加前导下划线（`neoflux_foo` →
  `_neoflux_foo`）。`.S` 内用宏隐藏该差异：

  ```asm
  #if defined(__APPLE__) && defined(__MACH__)
  #define SYMBOL_NAME(x) _##x
  #else
  #define SYMBOL_NAME(x) x
  #endif
  ```

  注意本目录使用**大写 `.S`**（走 C 预处理器），所以 `#if`/宏都可用。

## 如何被 CMake 选用（简述）

- 本目录 **不** 用 glob 自动收集；仓库里也没有 `src/native/CMakeLists.txt`，
  所有接线都在 `neoflux/CMakeLists.txt`。`enable_language(ASM)` 已在那里调用，
  新增 `.S` 时按用途放进对应的一个块，用 `target_sources(...)` 显式加入：
  - **"Platform-native tuning layer" 块**（如 `xgetbv.S`）：按**平台 + 处理器 +
    编译器**选择。xgetbv 仅在 **x86_64 且非 MSVC**（Linux/macOS GCC/Clang、
    MinGW/Clang-on-Windows）时编译；MSVC 走 `_xgetbv` intrinsic，不链接本文件。
  - **"SIMD kernels" 块**（如 `premultiply_rgba_*.S`）：按**目标架构**选择，
    不看操作系统——SSE2 是 x86-64 架构基线、NEON 是 AArch64 架构基线，因此
    无需运行时探测。MSVC（需 MASM 而非 GAS 语法）不编译，由
    `src/native/simd_kernels.cpp` 的可移植标量内核兜底。
- 选定了 `.S` 必须**同时**在 `neoflux/CMakeLists.txt` 里
  `target_compile_definitions(neoflux PRIVATE NEOFLUX_NATIVE_ASM_*=1)`，让
  `asm_symbols.h` 只在符号真的被链接时才声明它。
- CMake 集成由构建维护者负责；新增例程请同步更新 `asm_symbols.h` 注释
  与本节说明。

> 具体 CMake 改动见 `neoflux/CMakeLists.txt`，本文件仅描述约定。
