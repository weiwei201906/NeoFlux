# src/native/asm/ - hand-written assembly routines

**This directory holds the small set of performance-critical platform routines
that C++ and compiler intrinsics cannot express.**

The Chinese version of this document is kept with the other translated
documentation as `docs/zh/guide/native-asm-files.md`.

## What belongs here

- **Single-purpose standalone functions**: one `.S` file implements **one**
  function, named as described below.
- **Performance-critical bare instruction sequences**: cases that need exact
  control over instruction selection, register use, or that have to step past
  the boundary of what a compiler abstracts.
- **Instructions with no intrinsic**: when the target platform or compiler
  offers no builtin, a short hand-written sequence is allowed. `xgetbv.S` is
  exactly that case.

## What does not belong here

- **Inline asm**: `__asm__` / `__asm` are **forbidden** in C++ sources (a hard
  project rule). Platform-specific assembly is externalized into this
  directory instead.
- **Complex logic**: loops, branches and data-structure manipulation belong in
  C++ or intrinsics, not in `.S`. What is left here should be "a few
  instructions you can explain in one sentence". **The only exception is a
  SIMD kernel, see the next section** - `premultiply_rgba_*.S` contains a loop.
- **Business code**: rendering, media, windowing and tuning policy belong to
  their own modules and must not enter this directory.
- **Anything an intrinsic already covers**: when an equivalent intrinsic
  exists (MSVC `_xgetbv`, `__cpuid`), prefer the intrinsic. It is not inline
  asm and it is more portable.

## The one exception: SIMD kernels (loops are allowed)

"Do not put complex logic here" means **do not write business logic in
assembly**. One class of code is outside that rule:

**A SIMD kernel that is performance-critical and needs instruction
determinism, exact register control, and cross-compiler consistency.**

The test is: **if you wrote it with intrinsics, different compilers would
select different instructions and allocate different registers, and the
kernel's ABI behaviour or numeric result must not depend on that difference** -
then it belongs in hand-written assembly. `premultiply_rgba_x86_64.S` (SSE2)
and `premultiply_rgba_aarch64.S` (NEON) are that class: the same formula must
be byte-identical under GCC, Clang, MSVC and AppleClang, and must touch only
registers the caller does not have to save.

Beyond the general requirements above, such a file **must** also:

- [ ] Process a **fixed number of pixels per pass** (4 on x86-64, 8 on
      AArch64), state that number in the file header, and **return the number
      of pixels actually processed**. Tail pixels are never touched.
- [ ] Leave the tail to the portable scalar kernel in
      `src/native/simd_kernels.cpp`; assembly never has to handle "the last
      few".
- [ ] **Touch only caller-saved registers.** This is the easiest mistake to
      make: on Win64 `xmm6`-`xmm15` are callee-saved (on SysV they are not),
      so an x86-64 kernel uses **only `xmm0`-`xmm5`**; under AAPCS64 the low
      64 bits of `v8`-`v15` are callee-saved, so an AArch64 kernel uses
      **only `v0`-`v7` and `v16`-`v31`**. Code that breaks this passes on
      Linux and silently corrupts the caller's registers on Windows.
- [ ] Serve both ABIs (SysV AMD64 and Win64) from one instruction stream by
      selecting the argument registers with a macro.
- [ ] Build constants in place with ALU instructions (`pcmpeqd`, `psrlw`,
      `psllq`, `movi`) instead of a `.rodata` section, so the file stays
      position independent.
- [ ] Register the C ABI declaration in `asm_symbols.h` (**never** write
      `extern "C"` at the call site), and have CMake define the matching macro
      only when that `.S` is actually compiled in.
- [ ] Ship a cross-check test: `verify/test_native_simd.cpp` compares against
      **both** the scalar reference and the SSE2 intrinsic reference, and all
      three must agree byte for byte.

> The converse also holds: if the compiler already has a stable intrinsic for
> an operation and the result does not depend on instruction selection, do
> **not** write assembly. That falls under "what does not belong here" above
> and should be implemented with intrinsics in the platform layer
> (`src/native/`) without leaking into business code.

## Readability requirements (checklist)

- [ ] The file header comment contains: SPDX/copyright, the function contract
      (signature, semantics, return value), the ABI it follows (SysV AMD64 /
      Win64) and any key ABI notes.
- [ ] **Every instruction** has an inline comment saying what it does.
- [ ] One function per file, one responsibility.
- [ ] Symbols use the `neoflux_` prefix so they cannot collide with another
      library at link time.
- [ ] Platform and word-size constraints are stated (for example "64-bit only;
      i386 is not in the build matrix").
- [ ] Special handling such as Apple's leading underscore has an explicit
      comment explaining it.

## Symbol naming convention

- Every exported symbol uses the `neoflux_` prefix and C linkage
  (`extern "C"`).
- The C++ side declares them centrally in
  [`asm_symbols.h`](asm_symbols.h). Do **not** write an `extern "C"`
  declaration at the call site.
- **Apple exception**: Mach-O prefixes every C symbol with an underscore
  (`neoflux_foo` becomes `_neoflux_foo`). A macro hides that difference inside
  the `.S` file:

  ```asm
  #if defined(__APPLE__) && defined(__MACH__)
  #define SYMBOL_NAME(x) _##x
  #else
  #define SYMBOL_NAME(x) x
  #endif
  ```

  Note the **uppercase `.S`** used in this directory: it goes through the C
  preprocessor, so `#if` and macros are available.

## How CMake selects these files (summary)

- This directory is **not** collected with a glob, and there is no
  `src/native/CMakeLists.txt`. All wiring lives in `neoflux/CMakeLists.txt`,
  where `enable_language(ASM)` is already called. Add a new `.S` to exactly one
  block with an explicit `target_sources(...)`:
  - **The "Platform-native tuning layer" block** (for example `xgetbv.S`):
    selected by **platform, processor and compiler**. xgetbv is assembled for
    **any non-MSVC x86** target (Linux/macOS GCC or Clang,
    MinGW/Clang-on-Windows). MSVC uses the `_xgetbv` intrinsic and does not
    link this file.
  - **The "SIMD kernels" block** (for example `premultiply_rgba_*.S`):
    selected by **target architecture**, not by operating system. SSE2 is an
    architectural baseline of x86-64 and NEON is one of AArch64, so no runtime
    probe is needed. MSVC (which needs MASM rather than GAS syntax) does not
    compile them, and the portable scalar kernel in
    `src/native/simd_kernels.cpp` covers the fallback.
- Selecting a `.S` **also** requires
  `target_compile_definitions(neoflux PRIVATE NEOFLUX_NATIVE_ASM_*=1)` in
  `neoflux/CMakeLists.txt`, so that `asm_symbols.h` declares the symbol only
  when it is really linked in.
- CMake integration is the build maintainers' responsibility. A new routine
  must update the `asm_symbols.h` comments and this section in the same change.

> The concrete CMake edits live in `neoflux/CMakeLists.txt`; this file only
> describes the conventions.
