# Hand-Written Assembly Kernels

NeoFlux ships a small number of performance-critical kernels as hand-written
assembly in `neoflux/src/native/asm/`. They are reached through the plain C ABI
and never through inline asm in a C++ source. This page explains why those
kernels exist at all, which layers of the tree are allowed to touch SIMD, the
contract every kernel must honour, and the checklist for adding one.

Most contributors never need any of this. The only kernel-facing API is
`neoflux::native::PremultiplyRgba8()`, which is portable, has a scalar fallback
everywhere, and is what the rest of the framework calls.

## Why assembly and not intrinsics

An intrinsic is a *request*, not an instruction. The compiler chooses the
instruction sequence, the register allocation, and the scheduling, and every
compiler chooses differently. For a kernel that must produce byte-identical
output on GCC, Clang, MSVC and AppleClang, that freedom is the bug:

| | Intrinsics | Hand-written assembly |
| --- | --- | --- |
| Instruction selection | Each compiler decides | Fixed by the source file |
| Register allocation | Each compiler decides | Fixed by the source file |
| Cross-compiler output | Similar, not guaranteed identical | Byte-identical by construction |
| New toolchain | May silently change codegen | Same code, or a clear link error |
| Reading the kernel | Requires knowing the compiler | Requires reading one file |

Assembly is therefore the right tool when the kernel's *ABI behaviour* or its
*numeric result* cannot be allowed to depend on compiler choices. It is the
wrong tool when a stable intrinsic already exists and the result does not
depend on instruction selection -- that case belongs in the platform layer as
intrinsics (see [When not to write assembly](#when-not-to-write-assembly)).

::: tip The assembly is not a performance shortcut
The reason the premultiply kernels are assembly is determinism, not speed. A
compiler will happily vectorise a scalar loop; what it will not promise is that
it picks the same instructions and the same registers tomorrow, or on a
different compiler.
:::

## Directory and layering policy

SIMD must not leak into the layers that call it. Business code, the render
pipeline and the scheduler call `PremultiplyRgba8()` and are not allowed to
know that SSE2 or NEON exists.

| Tree | Intrinsic headers | Inline asm | Calls the C ABI kernels |
| --- | --- | --- | --- |
| `src/apps/`, `src/widgets/`, `src/core/`, business code | Forbidden | Forbidden | No (use `PremultiplyRgba8()`) |
| Render pipeline, scheduling | Forbidden | Forbidden | No (use `PremultiplyRgba8()`) |
| `src/native/` (dispatch, feature detect, platform) | Allowed | Forbidden | Yes, this is the dispatch layer |
| `src/native/asm/` | N/A (it is the assembly) | The only place for `.S` | N/A |
| `verify/`, `tests/` | Allowed, never shipped | Forbidden | Yes, for cross-checks only |

The rules are enforced by the `policy` job in CI:

- **No inline asm in C++** -- `__asm__`, `__asm` and `asm volatile` are rejected
  in every `.h`/`.cpp`. Hand-written routines live in `src/native/asm/` and are
  reached through `extern "C"` declarations. This is what keeps the kernels
  independent of any one compiler's inline-asm model.
- **No intrinsic headers outside `src/native/`, `tests/` and `verify/`** --
  `<immintrin.h>`, `<emmintrin.h>`, `<arm_neon.h>`, `<intrin.h>` and friends are
  rejected anywhere else. Line comments are stripped before matching, because
  the rule is documented in comments that spell out the very tokens being
  banned.
- **Assembly ABI contract** -- every `.S` must export a `neoflux_`-prefixed
  symbol that `asm_symbols.h` actually declares, and must respect the register
  discipline below.

::: warning Intrinsics in tests are deliberate
`verify/test_native_simd.cpp` includes `<emmintrin.h>`. A kernel checked only
against the dispatch layer's own scalar kernel would let a bug in the shared
arithmetic hide itself, so the test compares against a *second, independent*
intrinsic oracle. That file compiles into a throwaway binary and never ships.
:::

### When not to write assembly

If the operation already has a stable intrinsic and the result does not depend
on which instructions the compiler picks, do not write assembly. Put it in the
platform layer (`src/native/`) with intrinsics and keep it there. Two shipped
examples of exactly that:

- **cpuid** -- `__cpuid` / `__cpuidex`.
- **prefetch** -- `__builtin_prefetch` / `_mm_prefetch`.
- **xgetbv on MSVC** -- the `_xgetbv` intrinsic, which is why `asm/xgetbv.S` is
  linked only for non-MSVC x86-64 builds.

## Shipped kernels

| Symbol | File | ISA | Pixels per pass | Baseline? |
| --- | --- | --- | --- | --- |
| `neoflux_premultiply_rgba8` | `asm/premultiply_rgba_x86_64.S` | SSE2 | 4 | Yes (x86-64) |
| `neoflux_premultiply_rgba8` | `asm/premultiply_rgba_aarch64.S` | NEON / ASIMD | 8 | Yes (AArch64) |
| `neoflux_read_xcr0` | `asm/xgetbv.S` | x86-64 (no SIMD) | N/A | N/A |

Both premultiply files define the *same* symbol name. That is deliberate: CMake
links exactly one of them per target, so the dispatch layer in
`src/native/simd_kernels.cpp` stays architecture-agnostic and never has to name
an instruction set.

### Why there is no runtime CPU probe

`DetectCpuFeatures()` exists in `src/native/native_tuning.h` and reports
`sse42`, `avx2`, `neon` and `neon_fp16`. A kernel that needs AVX2 or SVE2 would
have to consult it at runtime, because those are optional extensions and the OS
can also refuse to enable them (XCR0 on x86, which is what `asm/xgetbv.S` reads;
`HWCAP` on AArch64).

The two shipped kernels are different. **SSE2 is an architectural baseline of
x86-64** and **Advanced SIMD (NEON) is an architectural baseline of AArch64**:
every implementation of those targets guarantees them. A runtime probe would
have nothing to decide and would only add a branch to a hot path, so selection
is purely compile-time, by target architecture.

## The kernel contract

Every kernel in `src/native/asm/` obeys the same contract:

1. **Fixed group size N** -- 4 pixels under SSE2, 8 under NEON. Stated in the
   file header.
2. **Returns the number of pixels converted** -- `floor(pixels / N) * N`.
3. **Never touches the tail** -- pixels past the returned count are left
   completely alone, so the caller can finish them with the scalar kernel
   without converting anything twice.
4. **Aliasing is safe** -- `dst` may equal `src`.
5. **No error path** -- null and zero guards belong to the dispatch wrapper
   (`PremultiplyRgba8()`), not to the assembly.

The formula, for channels `c` in `{R, G, B}`:

```
dst[c] = (src[c] * src[a] + 127) / 255
dst[a] = src[a]
```

### The divide by 255 is exact

The division is not a shift approximation. With `t = channel * alpha + 127`:

```
(t + (t >> 8)) >> 8   ==   floor(t / 255)   for every t in [0, 65152]
```

`channel * alpha` is at most 65025, so `t` is at most 65152, and every
intermediate value stays below 65406 -- the 16-bit lanes never overflow and no
widening to 32 bits is needed. Do not "simplify" this to a shift by 8 alone:
that is off by one for every value the rounding term pushes across a boundary.

## ABI and register discipline

This is the rule that is easiest to break and hardest to notice. A kernel that
clobbers a callee-saved register passes every test on the ABI where that
register is caller-saved, and silently corrupts its caller on the other one.

| Target | Callee-saved vector regs | Registers a kernel may use |
| --- | --- | --- |
| x86-64 (SysV AMD64 and Win64) | Win64: `xmm6`-`xmm15`. SysV: none. | `xmm0`-`xmm5` only |
| AArch64 (AAPCS64) | Low 64 bits of `v8`-`v15` | `v0`-`v7` and `v16`-`v31` |

The intersection of the two 64-bit x86 ABIs is `xmm0`-`xmm5`, so the x86-64
kernel restricts itself to exactly those six. Touching `xmm6` or above is the
classic way a hand-written kernel passes on Linux and destroys caller state on
Windows.

The shipped AArch64 kernel uses `v0`-`v7` plus `v16` and `v17`. CI rejects
`v8`-`v15` in any `*_aarch64.S` file.

Other ABI facts the kernels rely on:

- **Integer arguments** differ between the two 64-bit x86 ABIs (SysV uses
  `RDI`/`RSI`/`RDX`, Win64 uses `RCX`/`RDX`/`R8`). A macro hides the difference
  so one instruction stream serves both.
- **Caller-saved GPRs only** -- the x86-64 kernel uses `R10` as the loop counter
  and `RAX` for the return value; the AArch64 kernel uses `X3` and `X9`. Nothing
  needs saving or restoring.
- **Apple symbol mangling** -- Mach-O prepends an underscore to C symbols. A
  `SYMBOL_NAME()` macro hides it, which is why these files use uppercase `.S`
  (they go through the C preprocessor).
- **No `.rodata`** -- constants are built with ALU ops (`pcmpeqd` / `psrlw` /
  `psllq` / `movi`) so the file stays position independent.

## Adding a kernel

Work through this list in order. Steps 2 to 6 are each enforced by CI, so a
skipped step is a red build rather than a silent omission.

1. **Decide whether it should be assembly at all.** If a stable intrinsic exists
   and the result does not depend on instruction selection, stop and put it in
   the platform layer instead.
2. **Write the `.S`** in `neoflux/src/native/asm/`, one function per file,
   `neoflux_`-prefixed, with a file-header contract, the ABI it follows, a
   comment on every instruction, and the register discipline above. Keep the
   loop shape of the existing kernels: fixed group, return the count, never
   touch the tail.
3. **Declare it in `asm/asm_symbols.h`** under `extern "C"`, guarded by a
   `NEOFLUX_NATIVE_ASM_*` macro. Do not write `extern "C"` at the call site.
   The CI gate checks that every `.S` exports a symbol this header declares, so
   an unregistered file cannot silently never be linked.
4. **Select it in `neoflux/CMakeLists.txt`** by target architecture, with an
   explicit `target_sources(...)` and a matching private
   `target_compile_definitions(...)`. There is no glob. MSVC is excluded: it
   assembles MASM (`.asm`), not GAS (`.S`), and shipping a MASM translation
   nobody can verify here is worse than falling back.
5. **Wire the dispatch** in `src/native/simd_kernels.{h,cpp}`: add a
   `SimdLevel` enumerator, the group size, the compile-time dispatch, and a
   scalar tail path. Guard the call to the C ABI symbol with `#if defined(...)`,
   **not** with `if (group != 0)` -- a constant-false runtime condition still
   leaves a link-time reference in builds that compile no assembly, and no
   compiler promises to erase it.
6. **If the instruction set is not an architectural baseline**, gate it on
   `DetectCpuFeatures()` (plus XCR0 / HWCAP where the OS can refuse) and keep
   the scalar path reachable. The shipped kernels need none of this.
7. **Add cross-checks** to `verify/test_native_simd.cpp` and the compile line to
   `verify/README.md`.
8. **Document it** here and, if the kernel is user-visible, in the relevant
   guide page.

::: warning Two independent switches are two chances to disagree
CMake defines exactly one architecture macro per kernel and the umbrella
`NEOFLUX_NATIVE_ASM_PREMULTIPLY` is derived from it in `asm_symbols.h`.
Defining both separately would let them disagree, and a disagreement here is a
link error, not a fallback.
:::

## Build configurations

| Configuration | Kernel in use | `ActiveSimdLevel()` |
| --- | --- | --- |
| GCC / Clang / AppleClang on x86-64 | `premultiply_rgba_x86_64.S` | `kSse2` |
| GCC / Clang / AppleClang on AArch64 | `premultiply_rgba_aarch64.S` | `kNeon` |
| MSVC (any architecture) | none, scalar only | `kScalar` |
| Any other processor | none, scalar only | `kScalar` |

The scalar kernel in `simd_kernels.cpp` is always compiled in: it is both the
tail finisher and the entire implementation when no assembly was linked. A build
without a kernel is a supported configuration, not a broken one -- the CMake
configure step prints which path it picked.

## Verifying

`verify/test_native_simd.cpp` is the regression suite. It compiles without
CMake, tgfx, mpv or taitank. From `verify/README.md`:

```bash
SIMD_ASM="$REPO/neoflux/src/native/asm/premultiply_rgba_x86_64.S"
g++ -std=c++20 -O1 -DNEOFLUX_NATIVE_ASM_PREMULTIPLY_X86_64=1 "${INC[@]}" \
    "$V/test_native_simd.cpp" \
    "$REPO/neoflux/src/native/simd_kernels.cpp" "$SIMD_ASM" \
    -o /tmp/t11 && /tmp/t11
```

On arm64 use `-DNEOFLUX_NATIVE_ASM_PREMULTIPLY_AARCH64=1` and
`premultiply_rgba_aarch64.S`. Omit both and the suite still runs: the assembly
cases report `SKIP` and the scalar path is fully exercised.

| Case | Kind | What it proves |
| --- | --- | --- |
| 1 | smoke | Hand-picked pixels, both degenerate alphas (`0` collapses RGB to zero, `255` is exactly identity) |
| 2 | smoke | Every tail length `0..40`, plus alpha 0 and alpha 255 sweeps |
| 3 | stress | 65536 random pixels (256 KiB) with sentinel guard bytes behind every buffer |
| 4 | death | Hostile arguments (null pointers, `SIZE_MAX`, a count shorter than one group) never crash; runs in a forked child on POSIX |
| 5 | smoke | In-place `dst == src` matches the out-of-place result |
| 6 | oracle | The raw C ABI kernel matches the scalar reference, and writes nothing past its returned count |
| 7 | oracle | The assembly matches a second oracle written with SSE2 intrinsics |

Every buffer carries a sentinel band, so a kernel that walks one group too far
fails loudly instead of quietly scribbling past the end.

## See also

- [Native Tuning Layer](./native-tuning.md) -- the platform layer around these
  kernels: `DetectCpuFeatures()`, cache topology, thread scheduling.
- [Cross-Platform](./cross-platform.md) -- how per-platform translation units
  (and these `.S` files) are selected at configure time.
- [Testing](./testing.md) -- the smoke / death / stress categories used above.
- `neoflux/src/native/asm/README.md` -- the contributor-facing version of these
  rules, including readability requirements and the symbol naming convention.

---

*Available since the first hand-written SIMD kernel (SSE2 / NEON premultiply).*
