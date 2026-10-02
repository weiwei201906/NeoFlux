# Media Player

Demonstrates `MediaWidget`: a libmpv-backed video surface with a play/pause
toggle. The demo plays a local file or URL, composites the decoded frame into
the widget tree via an OpenGL texture, and exposes the most common playback
controls.

::: tip Prerequisite
This example is only built when libmpv was found at configure time. On Windows,
`scripts/download_mpv.ps1` fetches the latest MSVC build automatically. On Linux,
install `libmpv-dev` via your package manager.
:::

## Run

```powershell
# Build with examples enabled
cmake -S . -B build -G Ninja -DNEOFLUX_BUILD_EXAMPLES=ON
cmake --build build --target media_player_demo

# Play a file (positional argument)
.\build\bin\media_player_demo.exe C:\path\to\video.mp4

# Or via gflag
.\build\bin\media_player_demo.exe --source=http://example.com/stream.mpd
```

If no source is given, the demo looks for `assets/media/sample.mp4` next to
the executable.

## Features

- Video decoded by libmpv and rendered to an OpenGL texture
- Texture shared across the App/Render thread boundary (mpv frame callback
  wakes the render thread; `Paint` only reads an atomically-published texture id)
- Play/pause toggle button with label swap
- Tap-on-video to toggle playback
- Placeholder black background before the first frame arrives

## Key Code

```cpp
auto player = std::make_shared<MediaWidget>();
player->SetSource("video.mp4");

auto toggle = std::make_shared<Button>("Play");
toggle->SetOnPressed([player, toggle]() {
  if (player->GetState() == MediaState::kPlaying) {
    player->Pause();
    toggle->SetLabel("Play");
  } else {
    player->Play();
    toggle->SetLabel("Pause");
  }
});
```

## Threading Model

`MediaWidget` is the integration point between three threads:

| Thread | Responsibility |
| --- | --- |
| App / EventLoop | Layout, pointer events, `Paint` (reads texture id only) |
| Render | Owns the GL context; uploads mpv frames to an FBO-backed texture |
| mpv internal | Decodes video; calls `OnRenderUpdate` when a new frame is ready |

The mpv callback only signals the render thread (`condition_variable::notify_one`);
it never touches GL or blocks. See [Media Architecture](../guide/media.md) for
the full diagram.
