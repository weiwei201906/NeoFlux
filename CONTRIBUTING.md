# Contributing to NeoFlux

This document is the contract for every change that lands in this repository.
It is written to be mechanically checkable: each rule states what is required
and how a reviewer or a CI job can verify it. If a rule cannot be verified,
it does not belong here.

Corrections to this file are welcome, but they must keep the same shape: rule,
rationale, verification.

---

## 1. Repository layout you are changing

| Path | What it is | Who owns it |
|---|---|---|
| `neoflux/include/neoflux/` | Public framework headers (`#include <neoflux/...>`). | API review required |
| `neoflux/src/` | Framework implementation (core, widgets, renderers, media, native). | Framework owners |
| `neoflux/src/native/` | The only place framework code may call OS APIs directly. | Native owners |
| `neoflux/src/native/asm/` | Hand-written assembly kernels plus `asm_symbols.h`. | Native owners |
| `neoflux/tests/` | GoogleTest unit and integration tests wired through CTest. | Test owners |
| `src/` | The quick-start application, not the framework library. | App owners |
| `verify/` | Standalone correctness suite, compiled by hand, not by CMake. | Kernel owners |
| `examples/` | Example applications built with `-DNEOFLUX_BUILD_EXAMPLES=ON`. | Example owners |
| `thirdparty/` | Pinned dependencies. Only `thirdparty/CMakeLists.txt` and vendored sources we own are editable. | Build owners |
| `cmake/` | Build modules (`CompilerFlags.cmake`, `DownloadMPV.cmake`). | Build owners |
| `scripts/` | Bootstrap, vendor sync, asset download helpers. | Build owners |
| `.github/workflows/ci.yml` | The CI contract. Changing it changes what "green" means. | Build owners |

Rules that apply to "source" below cover `neoflux/`, `src/`, `verify/`,
`examples/`, `cmake/`, `scripts/`, and `.github/`. `thirdparty/` is governed
by its upstream projects except for the vendored sources this repository owns
and the glue in `thirdparty/CMakeLists.txt`.

---

## 2. Commit messages

**Rule**

- Every commit message follows the project's Conventional Commits form:
  `type(scope): subject`, where `type` is one of `feat`, `fix`, `refactor`,
  `perf`, `test`, `docs`, `build`, `ci`, `chore`, `style`, and `scope` names
  the area (`native`, `render`, `media`, `widgets`, `core`, `cmake`, `ci`,
  `docs`, ...).
- The subject line is English and ASCII only, in the imperative mood, with no
  trailing period.
- The body describes three things, in this order: what changed, why it
  changed, and how to verify it. "How to verify" names the exact command or
  test, not "run CI".
- The body is wrapped at 72 columns and is separated from the subject by a
  blank line.
- A commit that changes behaviour without touching its tests states in the
  body why no test change was required.

**Verification**

- `git log --format=%s -<n>` subjects match
  `^(feat|fix|refactor|perf|test|docs|build|ci|chore|style)\([a-z0-9/-]+\): .+$`.
- `git log --format=%s -<n> | grep -P '[^\x00-\x7F]'` prints nothing.
- The pull request description repeats the verification command so a reviewer
  can reproduce it without reading the diff.

---

## 3. ASCII-only source paths

**Rule**

- Every version-controlled text file under the source paths listed in section 1
  is ASCII only (`0x00`-`0x7F`). This includes C++ sources and headers,
  assembly, CMake, shell and PowerShell scripts, workflow files, Markdown, and
  JSON.
- This explicitly includes Chinese characters, full-width punctuation, emoji,
  non-breaking spaces, curly quotes, en/em dashes, and any other non-ASCII
  whitespace or typography.
- If external text must be referenced from source, use ASCII-safe identifiers
  (for example `kDefaultFontName`) and keep the human-readable text in the
  documentation tree.

**Scope: translated documentation is exempt and stays bilingual**

The rule applies to the paths a compiler or build tool reads, which is exactly
the list in section 1. It does **not** apply to `docs/` (including `docs/zh/`),
`README.md`, or `README-zh.md`: those are documentation assets, they may be
written in any language, and they may contain any non-ASCII character.

The consequence for a source directory is about placement, not about language.
A Chinese explanation of `neoflux/src/native/` belongs in
`docs/zh/guide/native-layer.md`, and the `README.md` that sits next to the code
stays in English so that the directory a reviewer opens is readable by every
maintainer and safe for every tool.

**Rationale**

Some assemblers and preprocessors treat non-ASCII bytes in comments as
invalid input, and a dash that looks like a hyphen in a review diff is
invisible until a compiler rejects the file on one platform only. Restricting
the rule to the paths a toolchain consumes removes that entire class of
platform-specific build failure without giving up translated documentation.

**Verification**

```bash
grep -rPn '[^\x00-\x7F]' \
  --include='*.h' --include='*.hpp' --include='*.c' --include='*.cc' \
  --include='*.cpp' --include='*.S' --include='*.asm' \
  --include='*.cmake' --include='CMakeLists.txt' \
  --include='*.sh' --include='*.ps1' --include='*.py' \
  --include='*.yml' --include='*.yaml' --include='*.json' --include='*.md' \
  neoflux src verify examples cmake scripts .github
```

The command must print nothing. The CI `Policy gates (Linux)` job runs the
source subset of this check; the full sweep is a review responsibility.

---

## 4. Pre-merge code hygiene

**Rule**

- Types are minimal: no redundant typedefs or using-aliases, no wrapper
  classes that only forward, no template that is instantiated once with a
  single concrete type.
- Every member function that does not mutate observable state is `const`.
- Every member function that does not depend on instance state is `static`.
- No syntactic sugar that hides intent. A named helper that is longer than the
  expression it wraps must earn its name in the pull request description.
- No dead code, no commented-out blocks, no `#if 0`, no unused parameters,
  no unused local variables, no unused private members.
- No `TODO` without an issue reference in the same comment.
- Every `// NOLINT` is justified in the diff or the pull request: which check
  it silences and why the code cannot satisfy the check. It is scoped to the
  narrowest possible range (one line, or one declaration), never to a whole
  file.

**Verification**

- `clang-tidy` passes (section 6).
- `grep -rn 'TODO' neoflux src` shows either no hits or hits with an issue
  reference.
- The compiler build is warning-clean with the project flags
  (`cmake/CompilerFlags.cmake`); `neoflux` and `neoflux_app` additionally
  compile with `-Werror` under GCC and Clang
  (`neoflux/CMakeLists.txt`, "Strict warnings").

---

## 5. Intrinsic and assembly policy

**Rule**

- Business, render-pipeline, and scheduling code must not include
  `<immintrin.h>`, `<arm_neon.h>`, `<xmmintrin.h>`, `<emmintrin.h>`,
  `<pmmintrin.h>`, `<tmmintrin.h>`, `<smmintrin.h>`, `<nmmintrin.h>`,
  `<wmmintrin.h>`, `<avxintrin.h>`, `<avx2intrin.h>`, or `<intrin.h>`.
- C++ sources must not contain inline assembly: no `__asm__`, no `__asm`, no
  `asm volatile`.
- A performance-critical kernel whose instruction selection must be
  deterministic is hand-written assembly under `neoflux/src/native/asm/`,
  reached through the plain C ABI declared in
  `neoflux/src/native/asm/asm_symbols.h`.
- Intrinsics are allowed only in: the dispatch / feature-detection layer
  (`neoflux/src/native/`, platform-specific subdirectories), test code, and
  fallback implementations. They must never leak into business logic.
- Every `.S` file exports exactly one `neoflux_`-prefixed symbol, and that
  symbol is declared in `asm_symbols.h`.
- x86-64 kernels touch only `xmm0`-`xmm5`: `xmm6`-`xmm15` are callee-saved on
  Win64 and touching them corrupts the caller on Windows only.
- AArch64 kernels touch only `v0`-`v7` and `v16`-`v31`: the low 64 bits of
  `v8`-`v15` are callee-saved under AAPCS64.

**Verification**

The `Policy gates (Linux)` job in `.github/workflows/ci.yml` enforces all of
the above: inline asm, intrinsic headers outside the allowed trees, the
`neoflux_` symbol registration in `asm_symbols.h`, and the callee-saved
register discipline. Run the same checks locally before pushing:

```bash
sed -n '/No inline asm/,/^  [a-z]/p' .github/workflows/ci.yml   # read the gates
```

---

## 6. clang-tidy

**Rule**

- All C++ changes pass clang-tidy with this repository's `.clang-tidy`
  configuration. The configuration is authoritative; do not weaken it in the
  same pull request that adds the code it rejects.
- `neoflux/src/native/asm/` is excluded from clang-tidy: it is assembly, and
  the C++ checks do not apply to it.
- Test-only intrinsic code may use a directory-level exemption; the exemption
  is named in the pull request.
- New `// NOLINT` comments are justified and scoped to the smallest possible
  range (section 4).
- clang-tidy is run through the compile database, so it analyses the same
  translation units, defines, and include paths as the real build.

**Verification**

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DNEOFLUX_BUILD_TESTS=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
run-clang-tidy -p build -j"$(nproc)" \
  -header-filter='(neoflux/include/neoflux|neoflux/src|src)/.*' \
  -quiet \
  '^(?!.*thirdparty/).*(/neoflux/src/|/src/)'
```

The command must report no diagnostics at error severity. The CI job
`clang-tidy (Linux)` fails the pull request otherwise.

---

## 7. CMake and assembly build gate

**Rule**

- Assembly support is enabled with `enable_language(ASM)` (or the appropriate
  `ASM_MASM` / `ASM_NASM` / `ASM_MARMASM` variant) in the directory that owns
  the assembly sources.
- MSVC x64 must not hand `.S` files to the C or C++ compiler. MSVC builds use
  MASM (`.asm`) or skip the assembly source; the fallback must be the portable
  scalar implementation, never a link error.
- A `.S` file is added to a target only on the platform and architecture whose
  ABI it implements, and the matching `NEOFLUX_NATIVE_ASM_*` definition is set
  in the same decision so that dispatch code and linked code cannot disagree.
- Every platform/compiler combination in the CI matrix configures, builds, and
  links with the assembly sources present.
- A new `.S` file that is not added to any target is a silent no-op; the pull
  request must show the CMake change that wires it in.
- Any CMake change that writes into `thirdparty/tgfx/` must be idempotent and
  must not be required for an offline build to configure.

**Verification**

- `cmake -S . -B build -G Ninja -DNEOFLUX_BUILD_TESTS=ON` configures on the
  platform you touched, and the configure output shows the expected
  "Native SIMD kernel: ..." status line from `neoflux/CMakeLists.txt`.
- `cmake --build build` links `neoflux` and `neoflux_app` with no undefined
  `neoflux_` symbols.
- On MSVC, `dumpbin /symbols` on the framework object files shows no reference
  to a symbol that only exists in a `.S` file.

---

## 8. Tests

**Rule**

- Smoke tests: basic initialization, dispatch, and kernel entry points must
  pass. A new dispatch entry point ships with a smoke test.
- Death tests: invalid parameters, null inputs, and unsupported feature paths
  must fail predictably, and the test asserts that specific failure. A new
  guard clause ships with a death test.
- Stress tests: hot kernels run under sustained load without crashes, data
  races, or regressions. A change to a hot path ships with its stress test
  updated or a measurement showing the path is unchanged.
- Death tests run in a Debug build, where `assert` and abort paths are active.
  A death test that only passes in Release is not a death test.
- Every new test is registered with CTest through `gtest_discover_tests`
  (`neoflux/tests/CMakeLists.txt`), so `ctest` finds it.

**Verification**

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DNEOFLUX_BUILD_TESTS=ON
cmake --build build -j
cd build && ctest --output-on-failure
```

On Linux, run the suite headless:

```bash
xvfb-run -a ctest --output-on-failure
```

---

## 9. Documentation

**Rule**

- Every new public API, dispatch entry point, and assembly kernel has matching
  documentation.
- Kernel and assembly documentation states the calling convention, the
  parameter layout (register or stack), the registers it clobbers, the
  registers it preserves, and the platforms it supports.
- When an assembly kernel replaces an intrinsic or scalar reference, the
  document names the reference implementation and the verification method that
  proves the two agree, including the input range over which they agree.
- A behaviour change updates the affected document in the same commit. A
  document that contradicts the code is a defect.
- Documentation is ASCII only (section 3).

**Verification**

- An assembly kernel header comment contains a "Contract" section listing
  arguments, return value, clobbers, and platforms; see
  `neoflux/src/native/asm/premultiply_rgba_x86_64.S` for the expected shape.
- The verify suite entry that cross-checks the kernel against the reference
  implementation is named in the pull request.

---

## 10. Pull request checklist

Copy this into the pull request description and tick every box.

```text
Commit message
[ ] Conventional Commits form, English ASCII subject, imperative mood
[ ] Body states what changed, why, and the exact verification command

ASCII
[ ] No non-ASCII bytes anywhere in the touched files
[ ] Any translated asset lives outside the source paths and is declared

Code hygiene
[ ] Types minimal; no redundant typedefs, wrappers, or single-use templates
[ ] const / static applied wherever the rule allows
[ ] No dead code, no commented-out blocks, no unused parameters or members
[ ] Every new // NOLINT justified and scoped to one line or declaration

Intrinsics and assembly
[ ] No intrinsic header outside neoflux/src/native/, tests, and verify
[ ] No inline asm in any C++ source
[ ] Every new .S registered in asm_symbols.h and wired into CMake
[ ] Callee-saved register discipline respected for the target ABI

clang-tidy
[ ] clang-tidy clean with the repository .clang-tidy configuration
[ ] neoflux/src/native/asm/ excluded from clang-tidy

CMake / ASM gate
[ ] enable_language(ASM) (or the MASM / NASM variant) used where needed
[ ] MSVC x64 does not compile .S through the C++ compiler
[ ] Configure + build + link verified on every platform touched

Tests
[ ] Smoke test added or updated for new entry points
[ ] Death test added or updated for new guard clauses, run in Debug
[ ] Stress test added or updated for hot-path changes
[ ] New tests registered with CTest

Documentation
[ ] Public API / dispatch / kernel documentation updated in the same commit
[ ] Calling convention, parameter layout, clobbers, and platforms documented
[ ] Reference implementation and verification method named for new kernels
```

---

## 11. Local build recipes

Configure and build the framework plus the quick-start application:

```bash
git submodule update --init --recursive
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DNEOFLUX_BUILD_TESTS=ON
cmake --build build -j
```

Useful options:

| Option | Default | Effect |
|---|---|---|
| `NEOFLUX_BUILD_TESTS` | `OFF` | Build the GoogleTest suite and register it with CTest |
| `NEOFLUX_BUILD_EXAMPLES` | `OFF` | Build the `examples/` applications |
| `NEOFLUX_ENABLE_ASAN` | `OFF` | AddressSanitizer for the whole build (non-MSVC) |
| `NEOFLUX_ENABLE_TSAN` | `OFF` | ThreadSanitizer for the whole build (non-MSVC) |
| `CMAKE_EXPORT_COMPILE_COMMANDS` | `ON` | Writes `build/compile_commands.json` for clang-tidy |

The GPU backend is tgfx's own compile-time choice, selected in
`thirdparty/CMakeLists.txt` (`TGFX_USE_OPENGL`, `TGFX_USE_METAL`,
`TGFX_USE_VULKAN`, `TGFX_USE_D3D12`). NeoFlux code contains no backend
branching of its own, and no OpenGL calls: all drawing and all GPU resource
ownership belong to tgfx.

---

## 12. Review etiquette

- Keep a pull request to one concern. A refactor and a behaviour change in the
  same pull request must be two commits, and preferably two pull requests.
- A reviewer may ask for a rule in this document to be applied; the author may
  propose changing the document instead, in a separate pull request.
- "It works on my machine" is not verification. Name the command and the
  platform.
