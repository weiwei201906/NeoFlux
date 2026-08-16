# NeoFlux third-party dependencies

This directory contains all third-party libraries used by NeoFlux.

## Managed via CMake FetchContent

Dependencies are automatically downloaded and built by CMake into
`_deps/` on first configure. No manual installation required.

| Library   | Purpose                          | Version |
|-----------|----------------------------------|---------|
| glog      | Logging                          | v0.7.1  |
| gflags    | Command-line flag parsing        | v2.2.2  |
| googletest| Unit testing                     | v1.14.0 |
| glfw      | Desktop window / input (desktop) | 3.4     |
| taitank   | Flexbox layout engine            | main    |
| tgfx      | 2D graphics rendering            | main    |

## Adding a new dependency

1. Add a `FetchContent_Declare` + `FetchContent_MakeAvailable` block in
   `CMakeLists.txt`.
2. Link the target in the root `CMakeLists.txt`.
3. Do **not** commit the `_deps/` directory (it is auto-generated).
