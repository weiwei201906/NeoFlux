# Third-Party Notices

This file lists external works incorporated into NeoFlux. NeoFlux framework
source under `neoflux/` and the user quick-start under `src/` is licensed
under GPL-3.0-or-later unless a file header says otherwise (see below).

## Adapted upstream components (derivative files carry their own SPDX header)

### EUI-Neo (sudoevolve/EUI-NEO) -- Apache License 2.0

The following four widgets were *designed with reference to* the EUI-Neo
component library. No EUI-Neo source lines were copied; behavior and structure
were adapted to the NeoFlux widget/render model. These files carry
`SPDX-License-Identifier: Apache-2.0` headers instead of the project-wide
GPL header:

- `neoflux/include/neoflux/widgets/card.{h}` + `neoflux/src/widgets/card.cpp`
- `neoflux/include/neoflux/widgets/checkbox.{h}` + `neoflux/src/widgets/checkbox.cpp`
- `neoflux/include/neoflux/widgets/progress_indicator.{h}` + `neoflux/src/widgets/progress_indicator.cpp`
- `neoflux/include/neoflux/widgets/switch.{h}` + `neoflux/src/widgets/switch.cpp`

Upstream referenced: https://github.com/sudoevolve/EUI-NEO
Upstream revision: `782c56993dc1890e0589e2100cfa74322bb0e0bf`
(Copyright sudoevolve; licensed under the Apache License, Version 2.0.)

## Bundled third-party libraries (in `thirdparty/`, retained under their own licenses)

These are used unmodified (or built via CMake FetchContent) and keep their own
license files inside `thirdparty/`. They are not relicensed by this project.

- **tgfx** -- Tencent, BSD-3-Clause. 2D vector rendering backend.
  https://github.com/Tencent/tgfx
- **Taitank** -- Tencent (THL A29 Limited), Apache-2.0. Flexbox layout engine.
  https://github.com/Tencent/Taitank
- **FreeType 2** -- FreeType License (FTL) or, at your option, GPLv2 with
  exceptions. Font rasterization.  https://www.freetype.org
- **glog** -- Google Inc., BSD-3-Clause. Application-level logging.
  https://github.com/google/glog
- **gflags** -- Google Inc., BSD-3-Clause. Command-line flag parsing.
  https://github.com/gflags/gflags
- **GoogleTest / GoogleMock** -- Google Inc., BSD-3-Clause. Unit testing.
  https://github.com/google/googletest
- **GLFW** -- zlib/libpng license (Marcus Geelnard / Camilla Lowy). Window and
  input.  https://www.glfw.org
- **mpv** (runtime binary in `thirdparty/mpv-bundle/`) -- LGPLv2.1 or later.
  Media playback backend. Upstream: https://mpv.io  (the bundled redistributable
  carries mpv's own license; no local text is vendored in this repo.)

## How to apply

Each NeoFlux-authored source file starts with one of:

- `// SPDX-License-Identifier: GPL-3.0-or-later` -- main project code, or
- `// SPDX-License-Identifier: Apache-2.0` -- the four EUI-Neo-adapted widgets.

When in doubt, the SPDX line at the top of a file is authoritative.
