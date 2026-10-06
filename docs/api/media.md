# Media

```cpp
class MediaWidget : public Widget;
enum class MediaState { kIdle, kLoading, kPlaying, kPaused, kEnded, kError };
```

Frame sharing without a GPU handle: the platform backend decodes video frames
into a CPU buffer, publishes the frame as a tgfx image under an opaque
frame-image id, and the render layer composites that image through whatever tgfx
backend is active (OpenGL, Metal, Vulkan, D3D12). NeoFlux itself makes no GL
call. On desktop this is libmpv (`MpvMediaPlayer`); on mobile it is the native
player.

::: warning Requires NEOFLUX_HAS_MPV
The desktop libmpv backend is compiled only when `NEOFLUX_HAS_MPV` is defined.
Without it, `CreateMediaPlayer()` returns `nullptr` and `MediaWidget` shows a
placeholder. See the [media guide](../guide/media.md).
:::

## MediaWidget

Header: `<neoflux/widgets/media_widget.h>`.

| Method | Signature | Notes |
|--------|-----------|-------|
| `SetSource` | `MediaWidget& SetSource(std::string_view)` | file path or URL. **(chainable)** |
| `GetSource` | `std::string_view GetSource() const noexcept` | |
| `Play` | `void Play()` | start/resume. |
| `Pause` | `void Pause()` | |
| `Stop` | `void Stop()` | reset to start. |
| `Seek` | `void Seek(double seconds)` | |
| `SetVolume` | `MediaWidget& SetVolume(double)` | 0.0 mute, 1.0 full. **(chainable)** |
| `GetVolume` | `double GetVolume() const noexcept` | |
| `GetPosition` | `double GetPosition() const noexcept` | seconds. |
| `GetDuration` | `double GetDuration() const noexcept` | seconds; 0 if unknown. |
| `GetState` | `MediaState GetState() const noexcept` | |
| `GetPlayer` | `MediaPlayer* GetPlayer() noexcept` | advanced control. |
| `SetBackgroundColor` | `MediaWidget& SetBackgroundColor(const Color&)` | placeholder. **(chainable)** |
| `SetTextColor` | `MediaWidget& SetTextColor(const Color&)` | placeholder text. **(chainable)** |

Tapping the widget toggles play/pause.

```cpp
auto media = std::make_shared<MediaWidget>();
media->SetSource("tests/data/sample.mp4")->SetVolume(0.8);
media->Play();
container->AddChild(media);
```

## MediaPlayer (abstract)

Header: `<neoflux/media/media_player.h>`. The decoupled backend interface.

```cpp
class MediaPlayer {
 public:
  virtual void SetSource(std::string_view) = 0;
  virtual void Play() = 0;
  virtual void Pause() = 0;
  virtual void Stop() = 0;
  virtual void Seek(double seconds) = 0;
  virtual void SetVolume(double) = 0;
  virtual double GetPosition() const noexcept = 0;
  virtual double GetDuration() const noexcept = 0;
  virtual MediaState GetState() const noexcept = 0;
  virtual int GetVideoWidth() const noexcept = 0;
  virtual int GetVideoHeight() const noexcept = 0;
  virtual void SetStateCallback(StateCallback) = 0;   // void(MediaState)
  virtual void SetFrameCallback(FrameCallback) = 0;   // void(uint32_t image_id, int w, int h)
  virtual void InitRender() = 0;                       // render thread, no GL context
  virtual uint32_t UpdateFrame() = 0;                  // opaque frame-image id (0 = none)
};
std::unique_ptr<MediaPlayer> CreateMediaPlayer();
```

::: warning Threading
`InitRender()` and `UpdateFrame()` must run on the **render thread** (the thread
that drives the render pump); no GPU/GL context is required. Business code only
calls the control methods.
:::

Frames are handed over as an opaque id, never as a GPU handle. The id is
resolved by the render layer through the internal frame-image registry, so the
same code path composites video on every tgfx backend.

## MpvMediaPlayer

```cpp
class MpvMediaPlayer final : public MediaPlayer;
```

Header: `<neoflux/media/mpv_media_player.h>`. The desktop libmpv software
render-API implementation: mpv renders into a CPU buffer, which is published as
a tgfx image. Pimpl: leaks no mpv, tgfx or GL types in the header. Use
`CreateMediaPlayer()` rather than constructing it directly.

## MediaState values

| Value | Meaning |
|-------|---------|
| `kIdle` | No source loaded. |
| `kLoading` | Loading/buffering. |
| `kPlaying` | Playing. |
| `kPaused` | Paused. |
| `kEnded` | Reached end of stream. |
| `kError` | Error. |
