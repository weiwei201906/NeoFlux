# Contributing

Thanks for your interest in contributing to NeoFlux! This document outlines the
coding standards and workflow expected for all pull requests.

## Prerequisites

Before submitting a PR, make sure you have:

- A C++20-capable compiler (GCC 11+, Clang 14+, MSVC 2022)
- CMake 3.20+
- clang-tidy (for static analysis)
- Git

## Coding Standards

### C++ Standard

NeoFlux targets **C++20**. Use modern C++20 features where appropriate:

- `std::string_view` for non-owning string parameters
- Designated initializers for structs
- Ranges and views (`std::views::filter`, `std::views::transform`)
- Concepts and constraints
- `std::span` for array views
- Coroutines (`Task<void>`) for async work

Reference: [C++20 on cppreference](https://en.cppreference.com/w/cpp/20)

### Google C++ Style Guide

NeoFlux follows the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
Key points:

- Names: `PascalCase` for classes/structs, `snake_case` for functions/variables,
  `kConstantName` for compile-time constants, `trailing_underscore_` for member
  variables
- Includes: project headers first, then third-party, then system
- Comments: English only, Doxygen-style for public APIs
- No `using namespace` in headers
- Lines should be under 80 characters where practical

### RAII and Smart Pointers

:::danger
Never use raw `new`/`delete` or raw owning pointers in new code.
:::

- Use `std::unique_ptr` for exclusive ownership
- Use `std::shared_ptr` for shared ownership (Widget tree uses this)
- Use `std::weak_ptr` for non-owning observers (especially in coroutines)
- Use `std::string_view` for non-owning string parameters
- Use RAII wrappers for all resources (file handles, GPU contexts, etc.)

Reference: [RAII on cppreference](https://en.cppreference.com/w/cpp/language/raii)
Reference: [Smart pointers on cppreference](https://en.cppreference.com/w/cpp/memory)

### Header-Only Declarations

:::warning
Headers (`.h`) must contain only declarations. All implementations go in `.cpp`
files. Template classes use `.inc` files with explicit instantiation in `.cpp`.
:::

This keeps compile times fast and hides implementation details.

### Pure ASCII

All source code, comments, log messages, and string literals must be pure ASCII.
No non-ASCII characters (including Chinese) are allowed in `.h`/`.cpp` files.

## Static Analysis

### clang-tidy

All PRs must pass clang-tidy with zero warnings. Run:

```bash
clang-tidy -p build src/**/*.cpp include/neoflux/**/*.h
```

The project ships a `.clang-tidy` configuration. Common checks enforced:

- `modernize-*` — modern C++ idioms
- `performance-*` — performance optimizations
- `readability-*` — code readability
- `bugprone-*` — common bug patterns
- `cppcoreguidelines-*` — C++ Core Guidelines

:::tip
If clang-tidy suggests a change that would harm readability or performance,
document the reason in the PR and add a targeted `NOLINT` comment.
:::

### Compiler Warnings

The build uses `-Wall -Wextra -Wpedantic -Werror`. All PRs must compile with
zero warnings.

## Testing

- Add unit tests for new functionality under `tests/`
- Run the full test suite before submitting:

```bash
cmake -S . -B build -DNEOFLUX_BUILD_TESTS=ON
cmake --build build
cd build && ctest --output-on-failure
```

## Before You Submit

:::danger
Do not submit a PR until you have verified everything locally.
:::

### Code Quality

- **Clean and optimal**: Review your code for unnecessary allocations,
  redundant copies, and missed optimization opportunities. Prefer
  `std::string_view` over `std::string` for non-owning parameters, use
  designated initializers for structs, and leverage C++20 ranges/views where
  appropriate.
- **Bitwise optimizations**: Replace integer division/modulo by powers of two
  with bit shifts and masks. Always comment the intent:
  - `x / 2` → `x >> 1` (integer halving)
  - `x % 2 == 0` → `(x & 1U) == 0U` (even/odd test)
  - `x % 64` → `x & 63U` (wrap at power-of-two boundary)
  - Ring queues use `mask_ = capacity - 1` with `index & mask_` instead of
    `index % capacity` (1 cycle AND vs ~20-40 for division).
  Floating-point division (`/ 2.0F`) **cannot** use bitwise shifts.
- **No raw owning pointers**: Use `std::unique_ptr` / `std::shared_ptr` /
  `std::weak_ptr`. Never use `new`/`delete` in new code.
- **RAII**: All resources (file handles, GPU contexts, memory) must be
  managed by RAII wrappers.
- **Pure ASCII**: All source code, comments, log messages, and string literals
  must be ASCII. No non-ASCII characters (including Chinese) in `.h`/`.cpp`.
- **Headers are declarations only**: All implementations go in `.cpp`. Template
  classes use `.inc` + explicit instantiation.

### Local Reproduction (Required)

Before opening a PR, you **must** verify locally:

1. **Clean build**: Delete your build directory and configure from scratch:
   ```bash
   rm -rf build
   cmake -S . -B build -DNEOFLUX_BUILD_TESTS=ON -DNEOFLUX_BUILD_EXAMPLES=ON
   cmake --build build
   ```
2. **Zero warnings**: Build must pass with `-Werror` (enabled by default).
3. **clang-tidy**: Run clang-tidy and fix all warnings:
   ```bash
   clang-tidy -p build src/**/*.cpp
   ```
4. **Tests pass**: All unit tests must pass:
   ```bash
   cd build && ctest --output-on-failure
   ```
5. **Examples run**: At minimum, run `hello_neoflux` and the example most
   relevant to your change to confirm it works at runtime, not just compiles.

PRs that fail any of these checks will be requested changes before review.

## Pull Request Workflow

1. Fork the repository and create a feature branch
2. Make your changes following the standards above
3. Run clang-tidy and fix all warnings
4. Build with `-Werror` and ensure zero warnings
5. Run the test suite
6. Verify examples run locally
7. Submit a PR with a clear description of the change

## Commit Messages

Use conventional commit format:

- `feat:` — new feature
- `fix:` — bug fix
- `docs:` — documentation only
- `refactor:` — code change that neither fixes a bug nor adds a feature
- `perf:` — performance improvement
- `test:` — adding or updating tests
- `chore:` — build process, tooling, or maintenance

## Getting Help

If you have questions about contributing, open an issue on GitHub.
