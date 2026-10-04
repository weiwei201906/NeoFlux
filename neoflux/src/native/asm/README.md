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
  `.S`。本目录只应剩下"几条指令就能说清楚"的东西。
- **业务代码**：渲染、媒体、窗口、调优策略等业务逻辑属于各自模块，
  不得进入本目录。
- **编译器 intrinsic 已经覆盖的能力**：若有等价 intrinsic（如 MSVC `_xgetbv`、
  `__cpuid`），优先用 intrinsic——它不算内嵌 asm，且更可移植。

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

- 本目录 **不** 由 `src/native/CMakeLists.txt` 之外的 glob 自动收集。
  新增 `.S` 时需在 `neoflux/CMakeLists.txt` 的 **"Platform-native tuning layer"**
  块里按平台/处理器显式 `target_sources(...)`，并确保工程已
  `enable_language(ASM)`。
- 选用条件由**平台 + 处理器 + 编译器**共同决定。以 `xgetbv.S` 为例：
  仅在 **x86_64 且非 MSVC**（Linux/macOS GCC/Clang、MinGW/Clang-on-Windows）
  时加入编译；MSVC 走 `_xgetbv` intrinsic，不链接本文件。
- CMake 集成由构建维护者负责；新增例程请同步更新 `asm_symbols.h` 注释
  与本节说明。

> 具体 CMake 改动见 `neoflux/CMakeLists.txt`，本文件仅描述约定。
