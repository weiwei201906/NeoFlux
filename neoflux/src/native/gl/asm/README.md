# native/gl/asm - Hand-written assembly for hot loops

This directory is a placeholder for platform-specific assembly that may be
needed later to optimize genuinely hot paths in the GL layer (e.g. fast
texture upload paths, swizzle loops). It ships no .s/.asm files today.

## Hard rule: no inline assembly in C++ sources

Never use `__asm__`, `asm ()`, or `_asm {}` inside a `.cpp`/`.h` file.
Inline asm:

- couples a translation unit to a specific compiler and architecture,
- defeats the optimizer's ability to schedule and inline around it,
- is untestable without the exact toolchain,
- and leaks compiler-specific syntax into portable framework code.

If a hot loop needs hand-tuned assembly, write it in a standalone assembly
file in THIS directory and assemble it through CMake.

## File naming and per-toolchain assembly

| Toolchain | Source extension | CMake enable language |
|-----------|------------------|-----------------------|
| GCC       | `.s` / `.S`      | `ASM`                 |
| Clang     | `.s` / `.S`      | `ASM`                 |
| MSVC      | `.asm`           | `ASM_MASM`            |

Conventions:

- One function per file, named `<loop>_<arch>.<ext>` (e.g.
  `upload_rgba_x86_64.s`, `upload_rgba_aarch64.S`).
- Expose a plain C ABI symbol and declare it `extern "C"` from the
  matching C++ translation unit. Do not mangle names manually.
- Save/restore every non-volatile register per the platform ABI
  (System V AMD64, Microsoft x64, AAPCS64). Do not assume red-zone
  availability when called from arbitrary C++ code.
- Keep the C++ fallback implementation always compiled in; select the asm
  path at runtime via CPU feature detection (e.g. XCR0/CPUID on x86,
  getauxval on ARM).

## Wiring from CMake

When an assembly file is added later, enable the right language per toolchain
inside `neoflux/CMakeLists.txt` and add the file to the `neoflux` target:

```cmake
if(MSVC)
  enable_language(ASM_MASM)
else()
  enable_language(ASM)
endif()
```

No assembly is enabled in this revision; the directory exists so future
hot-loop asm has an agreed, reviewed home and does not get inlined into
random .cpp files.
