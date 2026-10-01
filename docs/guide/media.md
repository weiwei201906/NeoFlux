# Media Playback (libmpv)

On desktop, NeoFlux can render video through [libmpv](https://mpv.io) using
mpv's **render API**. Instead of putting mpv in its own window, the renderer
is driven by NeoFlux's existing OpenGL context so each decoded video frame
becomes a normal GL texture that a widget can draw.

::: warning Optional feature
Desktop video support is gated behind the `NEOFLUX_HAS_MPV` compile definition
and requires libmpv development files at build time. Without it, the media
APIs are not compiled in and the rest of the framework works unchanged.
:::

## How it fits the two-layer model

```
+------------------ Application thread ------------------+
|  MediaPlayer widget                                     |
|    mpv_command(mpv, "loadfile", path)                   |
|    play / pause / seek                                  |
+--------------------------------------------------------+
                     |
                     |  mpv render context "need update" callback
                     v
+------------------ Render thread (owns GL context) -----+
|  mpv_render_context_render()  -> writes into a GL FBO    |
|  the resulting frame becomes a GL texture              |
|  that a Texture widget draws like any other command     |
+--------------------------------------------------------+
```

1. The **mpv render context** is created against the same OpenGL context the
   render thread already owns (the GLFW/WGL context on desktop).
2. mpv decodes the file asynchronously and calls an "update" callback when a
   new frame is ready. That callback asks the render thread to redraw.
3. On the render thread, `mpv_render_context_render()` draws the decoded frame
   into an off-screen GL framebuffer/texture. NeoFlux then emits an ordinary
   "draw texture" render command for that texture, so video composites exactly
   like any other widget (backgrounds, text overlays, clipping).

Because all mpv rendering happens on the render thread where the GL context is
current, no context migration is needed.

## Playing a real file

Playback takes a real media file path (or any URL mpv supports):

```cpp
// Built only when NEOFLUX_HAS_MPV is on.
MediaPlayer player;
player.SetSource("tests/data/sample.mp4");
player.Play();
```

Control uses mpv commands under the hood:

| Action | mpv command |
|--------|-------------|
| Load a file | `loadfile <path> replace` |
| Play / pause | `set pause no` / `set pause yes` |
| Seek | `seek <seconds> absolute` |
| Stop | `stop` |

## Sample test clip

The repo ships `tests/data/sample.mp4` as a short, self-contained clip used by
the media tests and demos. Point a `MediaPlayer` at it to exercise playback end
to end without downloading external assets:

```powershell
.\build\bin\<media_demo>.exe --logtostderr --verbose_logging
```

::: tip Use --logtostderr
mpv prints a lot of diagnostic output. Running with `--logtostderr` makes it
visible in the terminal (examples are otherwise GUI-subsystem with no console).
:::

## Requirements

- libmpv (`mpv/client.h`, `mpv/render_gl.h`) available at build time.
- The desktop GL path (`--render_backend=gl`), since the render context wraps
  an OpenGL context.
- A decode path for the container/codecs your files use (system ffmpeg/libav
  bundled with your mpv build).

## What could not be verified in this checkout

The `docs/bilingual` worktree does not currently contain the `MediaPlayer`
sources or the `NEOFLUX_HAS_MPV` CMake wiring. This page documents the intended
desktop media architecture described for the project; confirm the exact class
names and CMake option against the media sources in the main checkout before
shipping a product build.
