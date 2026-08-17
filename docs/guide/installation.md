# Installation

## Prerequisites

- **C++20 compiler**: GCC 10+, Clang 12+, or MSVC 2022+
- **CMake**: 3.20 or later
- **Git**: for fetching dependencies
- **Python**: 3.8+ (for some third-party builds)

### Platform-specific

**Windows**: MinGW-w64 or MSVC. The framework is tested with MinGW-w64 (GCC 15).

**Linux**: `build-essential`, `cmake`, `libgl1-mesa-dev`, `libx11-dev`,
`libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`, `libxi-dev`.

**macOS**: Xcode command line tools, CMake.

## Clone

```bash
git clone https://github.com/weiwei201906/NeoFlux.git
cd NeoFlux
```

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
| `NEOFLUX_BUILD_EXAMPLES` | `ON` | Build example applications |

### Building with Tests

```bash
cmake .. -DNEOFLUX_BUILD_TESTS=ON
cmake --build . -j
ctest --output-on-failure
```

## Third-Party Dependencies

All dependencies are fetched via CMake `FetchContent` and placed under
`thirdparty/_deps/` (excluded from git):

- **Taitank** — flexbox layout engine
- **tgfx** — 2D graphics rendering (mobile)
- **GLFW** — desktop window/input
- **FreeType** — font rasterization
- **glog** — logging
- **gflags** — command-line flags
- **googletest** — unit testing (tests only)

## Verify

```bash
./bin/hello_neoflux
```

You should see a window with "Hello NeoFlux" and interactive buttons.
