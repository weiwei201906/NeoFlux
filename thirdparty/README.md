# NeoFlux third-party dependencies

Every dependency is vendored here as a git **submodule** and built from
source via `add_subdirectory` (see `CMakeLists.txt`). A fresh clone with
`--recurse-submodules` builds fully offline; the exact pinned revision of
each library is reviewable in this repository.

```sh
git clone --recurse-submodules https://github.com/weiwei201906/NeoFlux
# or, after a plain clone:
git submodule update --init --recursive
```

## Submodules

| Library    | Purpose                                  | Upstream                       |
|------------|------------------------------------------|--------------------------------|
| tgfx       | 2D rendering backend (GPU abstractions)  | github.com/Tencent/tgfx        |
| taitank    | Flexbox layout engine                    | github.com/Tencent/taitank     |
| freetype   | TrueType/OpenType font rasterization     | github.com/freetype/freetype   |
| glog       | Logging                                  | github.com/google/glog         |
| gflags     | Command-line flags                       | github.com/gflags/gflags       |
| googletest | Unit testing                             | github.com/google/googletest   |
| glfw       | Desktop window / input (desktop only)    | github.com/glfw/glfw           |
| vendor_tools | tgfx's vendor helper CMake (stubbed)   | github.com/libpag/vendor_tools |

## Vendored in-tree (not submodules)

tgfx normally pulls these via its `depsync`/gclient tooling, which needs
network access. We vendor small, well-defined copies instead so offline CI
configures cleanly, and wire them into the `tgfx` target from
`CMakeLists.txt`:

| Directory       | Purpose                                                     |
|-----------------|-------------------------------------------------------------|
| `concurrentqueue` | Header-only lock-free MPMC queue (cameron314)             |
| `skcms`         | Color management (portable baseline build)                  |
| `highway`       | tgfx's SIMD core (headers + dispatch runtime)               |
| `pathkit`       | Path triangulation slice used by tgfx's PathRef             |
| `stubs/`        | Small platform stubs (e.g. MinGW system-font shim)          |

## Notes

- The GPU backend is chosen with tgfx's own `TGFX_USE_*` CMake options
  (exactly one per build; see the block in `CMakeLists.txt`).
- `mpv-bundle/` (if present) is a downloaded Windows libmpv package managed
  by `cmake/DownloadMPV.cmake` — it is not a submodule and not committed.
- `fonts/` holds the font files scanned at startup by `FontManager`.
