# Frequently Asked Questions

## General

### What is NeoFlux?

NeoFlux is a lightweight, cross-platform C++20 UI framework with a two-layer
architecture: an Application layer running business logic and the Taitank flex
layout engine, and a Render layer consuming render commands via tgfx (mobile)
or GLFW (desktop).

### Why C++20?

C++20 enables coroutines (`co_await`), concepts, `std::span`, designated
initializers, and `std::bit_ceil`. These are used throughout the framework
for clean, efficient code.

### Is NeoFlux production-ready?

NeoFlux is under active development. The core architecture (widget tree, flex
layout, render pipeline, input dispatch) is functional. See the GitHub issues
for current limitations.

## Building

### How do I build NeoFlux?

```bash
cmake -S . -B build
cmake --build build -j
```

Examples and tests are off by default. Enable them with:

```bash
cmake -S . -B build -DNEOFLUX_BUILD_EXAMPLES=ON -DNEOFLUX_BUILD_TESTS=ON
```

### Why is the build slow the first time?

NeoFlux uses FetchContent to download and build third-party dependencies
(glog, gflags, glfw, taitank, freetype, gtest). Subsequent builds are fast
because dependencies are cached in `thirdparty/_deps/`.

### How do I cross-compile?

Set the CMake toolchain file for your target platform. NeoFlux uses platform
macros (`NEOFLUX_PLATFORM_DESKTOP`, `NEOFLUX_PLATFORM_MOBILE`,
`NEOFLUX_PLATFORM_WINDOWS`, etc.) to select the appropriate backend.

## Rendering

### What render backends are supported?

- **Vulkan** (default) — via tgfx
- **OpenGL** — via tgfx/GLFW
- **CPU** — software rendering (fallback)

Select with `--render_backend=vulkan|gl|cpu`.

### Why is my window black?

Common causes:
1. **No font loaded** — Text widgets need a `.ttf`/`.otf` file in the font
   directory. Call `app.SetFontDir("fonts")` before `Init()`.
2. **No route pushed** — All examples require `RouteRegistry::RegisterRoute()`
   followed by `app.PushRoute("/")`.
3. **Window not exposed** — The first frame may need a resize or focus event
   on some platforms.

### Why is text garbled?

The font file does not contain the glyphs you are trying to render. Use a font
that supports your character set (e.g., NotoSansSC for CJK).

## Widgets

### How do I create a custom widget?

Inherit from `Widget` (or `StatelessWidget`/`StatefulWidget`) and override the
relevant virtual functions: `Build()`, `OnMeasure()`, `Paint()`, and event
handlers. See the [Widget System](./widgets) guide.

### What is the difference between `MarkNeedsBuild()` and `MarkFrameDirty()`?

- `MarkNeedsBuild()` — marks the widget for rebuild on the next frame
  (children may change).
- `MarkFrameDirty()` — triggers a layout/paint cycle without rebuilding
  widgets. Use this for paint-time changes (e.g., drag offsets, scroll
  position).

### Why doesn't my Draggable follow the cursor?

Ensure you are not calling `MarkNeedsBuild()` in `OnPointerMove()` — the drag
offset is paint-time only. The framework calls `MarkFrameDirty()` automatically
on pointer events.

## Layout

### Why is my widget not visible?

- The widget has zero size (check `OnMeasure()` return value).
- The widget is outside the viewport (check flex constraints).
- The widget is clipped by a `ScrollView` or `ClipRect`.

### How do I make a widget fill available space?

Use `Expanded` (sets `flex_grow`) or set `SetWidth(0)` with `flex_grow > 0`
in a flex container.

## Threading

### Is NeoFlux thread-safe?

The widget tree is only accessed from the main thread. The render layer runs
on a separate thread and communicates via the SPSC ring queue. Do not touch
widgets from the render thread.

### How do coroutines work?

NeoFlux provides a `Task<T>` coroutine type with `co_await Yield()` (next
frame) and `co_await Sleep(duration)`. Coroutines are scheduled on the main
thread by `EventLoop`. Use `std::weak_ptr` to guard against widget destruction
during `co_await`.

## Troubleshooting

### The application crashes on startup

Check:
1. Font directory exists and contains valid font files.
2. `Init()` is called before `Run()`.
3. A route is registered and pushed.
4. The render backend is supported on your platform.

### clang-tidy reports warnings

Run `clang-tidy -p build src/**/*.cpp` to check. The project aims for zero
warnings. See [Contributing](./contributing) for the full checklist.

### How do I enable verbose logging?

```bash
./my_app --verbose_logging=true
```

Logs are written to `logs/` by default. Use `--logtostderr=true` to output to
stderr instead.
