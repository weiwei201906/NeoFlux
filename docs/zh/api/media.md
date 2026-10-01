# 媒体播放

```cpp
class MediaWidget : public Widget;
enum class MediaState { kIdle, kLoading, kPlaying, kPaused, kEnded, kError };
```

Flutter 式纹理共享：平台后端把视频帧解码成 OpenGL 纹理，渲染层再把它合成到
widget 矩形中。桌面端为 libmpv（`MpvMediaPlayer`），移动端为原生播放器。

::: warning 需要 NEOFLUX_HAS_MPV
桌面 libmpv 后端仅在定义了 `NEOFLUX_HAS_MPV` 时编译。否则
`CreateMediaPlayer()` 返回 `nullptr`，`MediaWidget` 只显示占位。详见
[媒体指南](../guide/media.md)。
:::

## MediaWidget

头文件：`<neoflux/widgets/media_widget.h>`。

| 方法 | 签名 | 说明 |
|------|------|------|
| `SetSource` | `MediaWidget& SetSource(std::string_view)` | 文件路径或 URL。**（可链式）** |
| `GetSource` | `std::string_view GetSource() const noexcept` | |
| `Play` | `void Play()` | 开始/继续。 |
| `Pause` | `void Pause()` | |
| `Stop` | `void Stop()` | 复位到开头。 |
| `Seek` | `void Seek(double seconds)` | |
| `SetVolume` | `MediaWidget& SetVolume(double)` | 0.0 静音，1.0 最大。**（可链式）** |
| `GetVolume` | `double GetVolume() const noexcept` | |
| `GetPosition` | `double GetPosition() const noexcept` | 秒。 |
| `GetDuration` | `double GetDuration() const noexcept` | 秒；未知为 0。 |
| `GetState` | `MediaState GetState() const noexcept` | |
| `GetPlayer` | `MediaPlayer* GetPlayer() noexcept` | 高级控制。 |
| `SetBackgroundColor` | `MediaWidget& SetBackgroundColor(const Color&)` | 占位背景。**（可链式）** |
| `SetTextColor` | `MediaWidget& SetTextColor(const Color&)` | 占位文字。**（可链式）** |

点击 widget 可切换播放/暂停。

```cpp
auto media = std::make_shared<MediaWidget>();
media->SetSource("tests/data/sample.mp4")->SetVolume(0.8);
media->Play();
container->AddChild(media);
```

## MediaPlayer（抽象）

头文件：`<neoflux/media/media_player.h>`。与平台后端解耦的接口。

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
  virtual void SetFrameCallback(FrameCallback) = 0;   // void(uint32_t tex, int w, int h)
  virtual void InitRender() = 0;                       // 渲染线程，GL 上下文当前
  virtual uint32_t UpdateTexture() = 0;                // 返回 GL 纹理名
};
std::unique_ptr<MediaPlayer> CreateMediaPlayer();
```

::: warning 线程约束
`InitRender()` 与 `UpdateTexture()` 必须在**渲染线程且 GL 上下文当前**时调用。
业务代码只调用控制方法。
:::

## MpvMediaPlayer

```cpp
class MpvMediaPlayer final : public MediaPlayer;
```

头文件：`<neoflux/media/mpv_media_player.h>`。桌面 libmpv render-API 实现。
采用 pimpl，头文件不暴露任何 mpv/GL 类型。请用 `CreateMediaPlayer()` 创建，
而非直接构造。

## MediaState 取值

| 取值 | 含义 |
|------|------|
| `kIdle` | 未加载源。 |
| `kLoading` | 加载/缓冲中。 |
| `kPlaying` | 播放中。 |
| `kPaused` | 已暂停。 |
| `kEnded` | 播放结束。 |
| `kError` | 出错。 |
