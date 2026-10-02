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

## Threading model

Video playback spans three threads. The GL work always happens on the render
thread; mpv only ever *signals* that a frame is ready, it never pushes render
commands (the SPSC queue is preserved).

```
+-- App / UI thread (EventLoop) -------------------------------+
|  Widget tree, widget lifecycle.                               |
|  App-affine calls: SetSource / Play / Pause / Stop / Seek /   |
|  SetVolume / SetStateCallback / SetFrameCallback /            |
|  SetWakeCallback.                                             |
|                                                               |
|  MediaWidget::Paint()  -- NO GL --                           |
|    reads the atomically-published texture_id/w/h and emits a  |
|    DrawTexture command onto the SPSC queue.                   |
+---------------------------------------------------------------+
            ^ MarkFrameDirty() (repaint)        \
            |                                    \
+-- mpv internal thread ---------------------------------------+
|  OnRenderUpdate():                                             |
|    bump update_count_, set new_frame_=true, notify,           |
|    then call SetWakeCallback().                               |
|    NEVER blocks, NEVER touches GL.                            |
+---------------------------------------------------------------+
            | layer->Wake()
            v
+-- Render thread (owns the GL context) -------------------------+
|  On each wake, first run the render pump:                     |
|    InitRender() once, then UpdateTexture() ->                  |
|      mpv_render_context_update / mpv_render_context_render    |
|      into a cached FBO-backed texture (FBO and texture are    |
|      reused; texture storage is (re)allocated only on size     |
|      change). Publish texture_id/w/h to atomics.              |
|  Then drain the SPSC queue and execute commands               |
|  (including the DrawTexture for the video).                    |
+---------------------------------------------------------------+
```

### Wake chain

1. mpv decodes a frame on its internal thread and invokes
   `OnRenderUpdate`. That callback only bumps the observability counter, raises
   a `new_frame_` flag, and calls the installed wake callback.
2. The wake callback (installed by `MediaWidget` on first build) does two
   non-blocking things: `RenderLayer::Wake()` wakes the render thread to upload
   the new frame, and `Application::MarkFrameDirty()` wakes the App thread to
   repaint.
3. The render thread, woken by `Wake()`, runs the pump: `UpdateTexture()` calls
   `mpv_render_context_render()` into the cached FBO and publishes the new
   texture id. It then drains the SPSC queue and composites that texture.

The render thread therefore sleeps until either the App submits commands or mpv
signals a new frame; there is no fixed per-frame poll.

### Method affinity contract

| Thread | Methods |
|--------|---------|
| App / UI | `SetSource`, `GetSource`, `Play`, `Pause`, `Stop`, `Seek`, `SetVolume`, `SetStateCallback`, `SetFrameCallback`, `SetWakeCallback` |
| Render (GL context current) | `InitRender`, `UpdateTexture` |
| Any thread | `GetState`, `GetVideoWidth`, `GetVideoHeight`, `GetPosition`, `GetDuration`, `GetRenderUpdateCount` |

Callbacks:

- `StateCallback` fires on whichever thread pumped mpv events — the render
  thread in production (via `UpdateTexture`), but it may also be dispatched
  synchronously on the App thread from `Play()`/`Pause()`/`Stop()`. Never block
  or touch GL.
- `FrameCallback` fires on the render thread, synchronously at the end of
  `UpdateTexture()`, with the GL context current.
- `WakeCallback` fires on the mpv internal thread. It must be non-blocking and
  must not touch GL.

Do **not** swap a callback (`SetStateCallback`/`SetFrameCallback`/
`SetWakeCallback`) from inside a callback. `GetSource()`/`SetSource()` are
App-thread affine; do not read the source string from the render thread while
the App thread may be calling `SetSource()`.

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

- libmpv (`mpv/client.h`, `mpv/render_gl.h`) available at build time.
- The desktop GL path (`--render_backend=gl`), since the render context wraps
  an OpenGL context.
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

`MediaWidget` is a Flutter-style texture-sharing widget: decoding runs inside
mpv on its own thread, each frame becomes a GL texture composited by the
render thread, and `Paint()` on the UI thread just emits one `DrawTexture`
command. You build your own transport UI out of ordinary buttons/sliders on
top of it; the widget itself only handles tap-to-toggle by default.

## GL resource lifecycle and teardown

The mpv render context, the GL texture, and the FBO are all created on the
**render thread** (where the OpenGL context is current). They MUST be freed on
the same thread -- deleting a GL object on a thread without a current context
causes a hard crash.

`MediaWidget` handles this automatically in its destructor:

1. `SetRenderPump(nullptr)` -- stops the render thread from pulling new frames.
2. `player->Stop()` -- stops playback.
3. `RenderLayer::RunOnRenderThread([]{ player->TeardownRender(); })` --
   synchronously executes `TeardownRender()` on the render thread (which owns
   the GL context), blocks until it returns. This frees the mpv render context,
   GL texture, and FBO.
4. The `player` `unique_ptr` then destructs on the App thread, which only runs
   `mpv_terminate_destroy()` (non-GL mpv core teardown).

::: warning Do not call `delete` on a MediaPlayer from the App thread
If you own a `MediaPlayer` directly (not via `MediaWidget`), you must call
`TeardownRender()` on the render thread (via `RunOnRenderThread` or equivalent)
BEFORE destroying the player. Otherwise the destructor will try to free GL
objects on a thread with no current OpenGL context.
:::
