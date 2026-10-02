# 媒体播放器

演示 `MediaWidget`：基于 libmpv 的视频播放表面，附带播放/暂停切换按钮。本示例播放本地文件或 URL，将解码后的帧通过 OpenGL 纹理合成到控件树中，并暴露最常用的播放控制。

::: tip 前提条件
本示例仅在 configure 阶段找到 libmpv 时才会编译。Windows 上 `scripts/download_mpv.ps1` 会自动拉取最新 MSVC 构建；Linux 上请通过包管理器安装 `libmpv-dev`。
:::

## 运行

```powershell
# 开启 examples 构建
cmake -S . -B build -G Ninja -DNEOFLUX_BUILD_EXAMPLES=ON
cmake --build build --target media_player_demo

# 位置参数传入文件路径
.\build\bin\media_player_demo.exe C:\path\to\video.mp4

# 或通过 gflag
.\build\bin\media_player_demo.exe --source=http://example.com/stream.mpd
```

未指定源时，示例会在可执行文件旁寻找 `assets/media/sample.mp4`。

## 功能

- 视频由 libmpv 解码并渲染到 OpenGL 纹理
- 纹理跨 App/渲染线程共享（mpv 帧回调唤醒渲染线程；`Paint` 只读原子发布的纹理 id）
- 播放/暂停切换按钮，标签自动切换
- 点击视频区域切换播放状态
- 首帧到达前显示黑色占位背景

## 关键代码

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

## 线程模型

`MediaWidget` 是三条线程的交汇点：

| 线程 | 职责 |
| --- | --- |
| App / EventLoop | 布局、指针事件、`Paint`（只读纹理 id） |
| Render | 持有 GL context；将 mpv 帧上传到 FBO 纹理 |
| mpv 内部 | 解码视频；新帧就绪时回调 `OnRenderUpdate` |

mpv 回调只通过 `condition_variable::notify_one` 通知渲染线程，不碰 GL、不阻塞。完整架构图见[媒体架构](../guide/media.md)。
