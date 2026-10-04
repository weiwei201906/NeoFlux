# 媒体播放（libmpv）

桌面端，NeoFlux 可通过 [libmpv](https://mpv.io) 的**渲染 API** 渲染视频。
mpv 不另开窗口，而是由 NeoFlux 已有的 OpenGL 上下文驱动渲染：每帧解码后的视频
变成一张普通 GL 纹理，像其它内容一样由 widget 绘制。

::: warning 可选功能，且与 OpenGL 后端绑定
桌面视频支持受编译宏 `NEOFLUX_HAS_MPV` 控制，构建时需要 libmpv 开发文件。
未开启时媒体 API 不参与编译，框架其余部分照常工作。

媒体属于 **OpenGL 后端模块**：只有 `TGFX_USE_OPENGL` 是激活的 tgfx 后端时，
构建才会探测并链接 libmpv。在 Vulkan/D3D12/Metal 构建下，mpv 不下载、不链接、
不编译（`MpvMediaPlayer` 退化为空实现），`MediaWidget` 只渲染占位内容。这是
刻意设计：mpv→GL 纹理互操作在其他后端上没有对应实现，与其半可用，不如彻底
解耦。
:::

## 线程模型

视频播放横跨三条线程。所有 GL 工作都在渲染线程完成；mpv 只负责“有新帧了”
这个信号，绝不直接往渲染队列里塞命令（SPSC 架构保持不变）。

```
+-- 应用 / UI 线程（EventLoop）--------------------------------+
|  widget 树与 widget 生命周期。                                |
|  应用亲和调用：SetSource / Play / Pause / Stop / Seek /       |
|  SetVolume / SetStateCallback / SetFrameCallback /           |
|  SetWakeCallback。                                            |
|                                                               |
|  MediaWidget::Paint()  —— 不碰 GL ——                         |
|    读取原子发布的 texture_id/w/h，向 SPSC 队列                 |
|    发出一条 DrawTexture 命令。                                |
+---------------------------------------------------------------+
            ^ MarkFrameDirty()（触发重绘）      \
            |                                    \
+-- mpv 内部线程 ----------------------------------------------+
|  OnRenderUpdate():                                            |
|    更新计数、置 new_frame_=true、notify，再调用              |
|    SetWakeCallback()。绝不阻塞、绝不碰 GL。                   |
+---------------------------------------------------------------+
            | layer->Wake()
            v
+-- 渲染线程（持有 GL 上下文）----------------------------------+
|  每次被唤醒先跑渲染泵：                                       |
|    先 InitRender() 一次，再 UpdateTexture() ->                 |
|      mpv_render_context_update / mpv_render_context_render    |
|      渲染进缓存的 FBO 纹理（FBO 与纹理复用，纹理存储仅在      |
|      尺寸变化时重新分配）。把 texture_id/w/h 发布到原子变量。  |
|  然后排空 SPSC 队列并执行命令（含视频的 DrawTexture）。       |
+---------------------------------------------------------------+
```

### 唤醒链路

1. mpv 在内部线程解码出一帧，回调 `OnRenderUpdate`。该回调只做三件小事：
   累加观测计数、置 `new_frame_` 标志、调用已注册的 wake 回调。
2. wake 回调（由 `MediaWidget` 在首次 build 时安装）做两个非阻塞动作：
   `RenderLayer::Wake()` 唤醒渲染线程去上传新帧，`Application::MarkFrameDirty()`
   唤醒应用线程去重绘。
3. 渲染线程被 `Wake()` 唤醒后执行渲染泵：`UpdateTexture()` 调用
   `mpv_render_context_render()` 画进缓存的 FBO 并发布新纹理 id，随后排空
   SPSC 队列、合成该纹理。

因此渲染线程只在“应用提交命令”或“mpv 报新帧”时才醒来，不再固定轮询。

### 方法亲和约定

| 线程 | 方法 |
|------|------|
| 应用 / UI | `SetSource`、`GetSource`、`Play`、`Pause`、`Stop`、`Seek`、`SetVolume`、`SetStateCallback`、`SetFrameCallback`、`SetWakeCallback` |
| 渲染（GL 上下文 current） | `InitRender`、`UpdateTexture` |
| 任意线程 | `GetState`、`GetVideoWidth`、`GetVideoHeight`、`GetPosition`、`GetDuration`、`GetRenderUpdateCount` |

回调：

- `StateCallback` 在“泵送 mpv 事件”的那条线程触发——生产环境是渲染线程
  （经由 `UpdateTexture`），但 `Play()`/`Pause()`/`Stop()` 也可能在应用线程同步派发。
  不要阻塞、不要碰 GL。
- `FrameCallback` 在渲染线程触发，位于 `UpdateTexture()` 末尾、GL 上下文 current。
- `WakeCallback` 在 mpv 内部线程触发。必须非阻塞、不得碰 GL。

不要在回调内部再去更换回调（`SetStateCallback`/`SetFrameCallback`/
`SetWakeCallback`）。`GetSource()`/`SetSource()` 是应用线程亲和的：不要在应用线程
可能调用 `SetSource()` 的同时从渲染线程读 source 字符串。

由于所有 mpv 渲染都在持有 GL 上下文的渲染线程上发生，无需迁移上下文。

## 播放真实文件

播放接受真实媒体文件路径（或 mpv 支持的任意 URL）：

```cpp
// 仅在 NEOFLUX_HAS_MPV 开启时编译。
MediaPlayer player;
player.SetSource("tests/data/sample.mp4");
player.Play();
```

控制走底层 mpv 命令：

| 操作 | mpv 命令 |
|------|----------|
| 加载文件 | `loadfile <path> replace` |
| 播放 / 暂停 | `set pause no` / `set pause yes` |
| 拖动进度 | `seek <秒> absolute` |
| 停止 | `stop` |

## 示例测试片段

仓库附带 `tests/data/sample.mp4`，是一段短小、自包含的视频，供媒体测试与演示使用。
让 `MediaWidget`（或 `MediaPlayer`）指向它即可端到端验证播放，无需下载外部素材：

```powershell
.\build\bin\neoflux_app.exe --logtostderr --verbose_logging
```

::: tip 用 --logtostderr
mpv 会打印大量诊断信息。加 `--logtostderr` 才能在终端看到（应用默认是 GUI
子系统、无控制台）。
:::

## 依赖要求

- 构建期可用的 libmpv（`mpv/client.h`、`mpv/render_gl.h`）。
- OpenGL 构建（`TGFX_USE_OPENGL`，默认开启），因为渲染上下文包裹 OpenGL 上下文。
- 你的 mpv 构建所带 ffmpeg/libav 能解码所用封装/编码格式。

## 快速开始：一个完整播放器页面

脚手架应用内置了 `/media` 路由，演示完整流程：窗口中间一块 `MediaWidget`、
一个播放/暂停切换按钮、一个返回按钮。播放源取自 `--media_source`
（默认 `./assets/media/sample.mp4`）。

```powershell
# 指向任意 mpv 支持的文件或 URL：
.\build\bin\neoflux_app.exe --media_source=C:\videos\clip.mp4 --logtostderr
```

在首页点 **Open Media Player ->** 即可进入。等价的 widget 代码见
`src/views/media/media_view.cpp`：

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

`MediaWidget` 是 Flutter 风格的纹理共享 widget：解码在 mpv 自己的线程上跑，
每帧变成 GL 纹理由渲染线程合成，UI 线程的 `Paint()` 只发一条
`DrawTexture` 命令。你在它上面用普通按钮/滑块自己搭控制 UI；widget 默认
只处理点按切换播放暂停。

## GL 资源生命周期与销毁

mpv render context、GL texture、FBO 都在**渲染线程**创建（那里有当前
OpenGL context）。它们必须在同一线程销毁——在没有当前 GL context 的线程
上删 GL 对象会直接崩溃。

`MediaWidget` 析构时自动处理：

1. `SetRenderPump(nullptr)` — 停止渲染线程拉取新帧。
2. `player->Stop()` — 停止播放。
3. `RenderLayer::RunOnRenderThread([]{ player->TeardownRender(); })` —
   在渲染线程同步执行 `TeardownRender()`（持有 GL context），阻塞直到完成。
   这会释放 mpv render context、GL texture 和 FBO。
4. 随后 `player` 的 `unique_ptr` 在 UI 线程析构，只执行 `mpv_terminate_destroy()`
   （非 GL 的 mpv 核心销毁）。

::: warning 不要在 UI 线程直接 delete MediaPlayer
如果你直接持有 `MediaPlayer`（而非通过 `MediaWidget`），必须在析构前
在渲染线程调用 `TeardownRender()`（通过 `RunOnRenderThread`）。否则析构
函数会在没有当前 OpenGL context 的线程上尝试释放 GL 对象，导致崩溃。
:::
