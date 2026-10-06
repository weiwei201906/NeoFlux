# Media Playback (libmpv)

On desktop, NeoFlux can render video through [libmpv](https://mpv.io) using
mpv's **software render API** (`MPV_RENDER_API_TYPE_SW`). Instead of putting mpv
in its own window, the render layer drives mpv on the render thread: every
decoded frame is rendered into a CPU buffer, published as a tgfx image, and
composited with the same backend-agnostic draw path as any other image. NeoFlux
never touches OpenGL itself, so playback works on the OpenGL, Metal, Vulkan and
D3D12 tgfx backends alike.

::: warning Optional feature
Desktop video support is gated behind the `NEOFLUX_HAS_MPV` compile definition
and requires libmpv development files at build time (the build probes for the
`thirdparty/mpv-bundle` bundle on Windows and for a system libmpv via
pkg-config elsewhere). Without libmpv, `MpvMediaPlayer` is a no-op stub and
`MediaWidget` renders its placeholder, while the rest of the framework works
unchanged. The media backend is independent of the selected tgfx backend.
:::

## Threading model

Video playback spans three threads. Frame pulling and publishing always happen
on the render thread; mpv only ever *signals* that a frame is ready, it never
pushes render commands (the SPSC queue is preserved).

```
+-- App / UI thread (EventLoop) --------------------------------+
|  Widget tree, widget lifecycle.                               |
|  App-affine calls: SetSource / Play / Pause / Stop / Seek /   |
|  SetVolume / SetStateCallback / SetFrameCallback /            |
|  SetWakeCallback.                                             |
|                                                               |
|  MediaWidget::Paint()  -- NO GL --                            |
|    reads the atomically-published image_id/w/h and emits a    |
|    DrawImage command onto the SPSC queue.                     |
+---------------------------------------------------------------+
            ^ MarkFrameDirty() (repaint)        \
            |                                    \
+-- mpv internal thread ----------------------------------------+
|  OnRenderUpdate():                                            |
|    bump update_count_, set new_frame_=true, notify,           |
|    then call SetWakeCallback().                               |
|    NEVER blocks, NEVER touches GL/GPU state.                  |
+---------------------------------------------------------------+
            | layer->Wake()
            v
+-- Render thread (drives the frame pump) ----------------------+
|  On each wake, first run the render pump:                     |
|    InitRender() once, then UpdateFrame() ->                   |
|      mpv_render_context_update / mpv_render_context_render    |
|      into a reusable CPU frame buffer (reallocated only on    |
|      size change), copied into a tgfx Bitmap and published    |
|      as a tgfx Image under a stable frame-image id.           |
|  Then drain the SPSC queue and execute commands               |
|  (including the DrawImage that composites the frame).         |
+---------------------------------------------------------------+
```

### Wake chain

1. mpv decodes a frame on its internal thread and invokes
   `OnRenderUpdate`. That callback only bumps the observability counter, raises
   a `new_frame_` flag, and calls the installed wake callback.
2. The wake callback (installed by `MediaWidget` on first build) does two
   non-blocking things: `RenderLayer::Wake()` wakes the render thread to publish
   the new frame, and `Application::MarkFrameDirty()` wakes the App thread to
   repaint.
3. The render thread, woken by `Wake()`, runs the pump: `UpdateFrame()` calls
   `mpv_render_context_render()` into the CPU frame buffer and republishes the
   frame image. It then drains the SPSC queue and composites that image.

The render thread therefore sleeps until either the App submits commands or mpv
signals a new frame; there is no fixed per-frame poll.

### Method affinity contract

| Thread | Methods |
|--------|---------|
| App / UI | `SetSource`, `GetSource`, `Play`, `Pause`, `Stop`, `Seek`, `SetVolume`, `SetStateCallback`, `SetFrameCallback`, `SetWakeCallback` |
| Render (no GL context needed) | `InitRender`, `UpdateFrame`, `TeardownRender` |
| Any thread | `GetState`, `GetVideoWidth`, `GetVideoHeight`, `GetPosition`, `GetDuration`, `GetRenderUpdateCount` |

Callbacks:

- `StateCallback` fires on whichever thread pumped mpv events - the render
  thread in production (via `UpdateFrame`), but it may also be dispatched
  synchronously on the App thread from `Play()`/`Pause()`/`Stop()`. Never block
  or touch GPU/driver state.
- `FrameCallback` fires on the render thread, synchronously at the end of
  `UpdateFrame()`, and hands over the published frame-image id with its pixel
  dimensions.
- `WakeCallback` fires on the mpv internal thread. It must be non-blocking and
  must not touch GPU/driver state.

Do **not** swap a callback (`SetStateCallback`/`SetFrameCallback`/
`SetWakeCallback`) from inside a callback. `GetSource()`/`SetSource()` are
App-thread affine; do not read the source string from the render thread while
the App thread may be calling `SetSource()`.

Because all frame pulling happens on the render thread, no GPU context
migration is needed anywhere in the media path.

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
the media tests and demos. Point a `MediaWidget` (or `MediaPlayer`) at it to
exercise playback end to end without downloading external assets:

```powershell
.\build\bin\neoflux_app.exe --logtostderr --verbose_logging
```

::: tip Use --logtostderr
mpv prints a lot of diagnostic output. Running with `--logtostderr` makes it
visible in the terminal (the app is otherwise GUI-subsystem with no console).
:::

## Requirements

- libmpv (`mpv/client.h`, `mpv/render.h`) available at build time.
- Any tgfx backend: the software render path is backend-independent and makes no
  GL call of its own.
- A decode path for the container/codecs your files use (system ffmpeg/libav
  bundled with your mpv build).

## Quick-start example: a full player screen

The scaffolded app ships a `/media` route that demonstrates the whole flow:
a `MediaWidget` filling the middle of the window, a play/pause toggle, and a
back button. The source is taken from `--media_source` (default
`./assets/media/sample.mp4`).

```powershell
# Point the demo at any mpv-supported file or URL:
.\build\bin\neoflux_app.exe --media_source=C:\videos\clip.mp4 --logtostderr
```

From the home screen, click **Open Media Player ->** to reach it. The
equivalent widget code (see `src/views/media/media_view.cpp`):

```cpp
auto media = std::make_shared<neoflux::MediaWidget>();
media->SetSource(path).Play();

auto play_btn = std::make_shared<neoflux::Button>("Pause");
play_btn->SetOnPressed([media, play_btn]() {
  if (media->GetState() == neoflux::MediaState::kPlaying) {
    media->Pause();
    play_btn->SetLabel("Play");
  } else {
    media->Play();
    play_btn->SetLabel("Pause");
  }
});
```

`MediaWidget` is a frame-sharing widget without the texture: decoding runs inside
mpv on its own thread, each decoded frame becomes a CPU-backed tgfx image that
the render thread composites, and `Paint()` on the UI thread just emits one
`DrawImage` command carrying the opaque image id. You build your own transport
UI out of ordinary buttons/sliders on top of it; the widget itself only handles
tap-to-toggle by default.

## Frame image lifecycle and teardown

`InitRender()`, `UpdateFrame()` and `TeardownRender()` are all driven by the
**render thread**. Nothing in that path needs a current OpenGL context: the mpv
software renderer writes into a CPU buffer, tgfx owns the bitmap/image, and the
renderer uploads the image through whichever GPU backend is active.

`MediaWidget` handles the lifecycle automatically in its destructor:

1. `SetRenderPump(nullptr)` -- stops the render thread from pulling new frames.
2. `player->Stop()` -- stops playback, so no further frame is decoded.
3. `RenderLayer::RunOnRenderThread([]{ player->TeardownRender(); })` --
   synchronously executes `TeardownRender()` on the render thread and blocks
   until it returns. This frees the mpv render context, releases the published
   frame image from the registry and drops the CPU frame buffers. Running it on
   the render thread is what serializes the release against an in-flight frame
   pull; it is not a GL requirement.
4. The `player` `unique_ptr` then destructs on the App thread, which only runs
   `mpv_terminate_destroy()` for the remaining mpv core.

::: warning Do not call `delete` on a MediaPlayer from the App thread
If you own a `MediaPlayer` directly (not via `MediaWidget`), call
`TeardownRender()` on the render thread (via `RunOnRenderThread` or equivalent)
BEFORE destroying the player, so the teardown cannot race a frame pull.
:::
