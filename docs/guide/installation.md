# Installation

## Prerequisites

- **C++20 compiler**: GCC 10+, Clang 12+, or MSVC 2022+
- **CMake**: 3.20 or later
- **Git**: for cloning the repository and initializing submodules
- **Python**: 3.8+ (used by some third-party build scripts)

### Platform-specific

**Windows**: MinGW-w64 or MSVC. The framework is tested with MSVC 2022+.

**Linux**: `build-essential`, `cmake`, `libgl1-mesa-dev`, `libx11-dev`,
`libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`, `libxi-dev`.

**macOS**: Xcode command line tools, CMake.

## Clone

```bash
git clone https://github.com/weiwei201906/NeoFlux.git
cd NeoFlux
git submodule update --init --recursive   # fetch third-party dependencies
```

Third-party libraries (glog, gflags, glfw, taitank, freetype, gtest, tgfx)
are managed as **Git submodules** under `thirdparty/`. Run the submodule
command above once after cloning.

## Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | `Release` | Build configuration |
| `NEOFLUX_BUILD_TESTS` | `OFF` | Build unit tests |
| `NEOFLUX_ENABLE_CLANG_TIDY` | `OFF` | Run clang-tidy as a build step |
| `NEOFLUX_USE_TGFX` | `OFF` | Use the tgfx rendering backend (MSVC) |

Examples are not bundled; create your own application under `src/` (see the
Quick Start guide) and place fonts in `assets/fonts/`.

### Building with Tests

```bash
cmake .. -DNEOFLUX_BUILD_TESTS=ON
cmake --build . -j
ctest --output-on-failure
```

## Third-Party Dependencies

All dependencies are Git submodules under `thirdparty/`:

- **Taitank** — flexbox layout engine
- **tgfx** — 2D graphics rendering (mobile, optional)
- **GLFW** — desktop window/input
- **FreeType** — font rasterization
- **glog** — logging
- **gflags** — command-line flags
- **googletest** — unit testing (tests only)

## Verify

```bash
./bin/neoflux_app
```

You should see a window with "NeoFlux Quick Start" text.
