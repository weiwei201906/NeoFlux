# 媒体播放（libmpv）

桌面端，NeoFlux 可通过 [libmpv](https://mpv.io) 的**渲染 API** 渲染视频。
mpv 不另开窗口，而是由 NeoFlux 已有的 OpenGL 上下文驱动渲染：每帧解码后的视频
变成一张普通 GL 纹理，像其它内容一样由 widget 绘制。

::: warning 可选功能
桌面视频支持受编译宏 `NEOFLUX_HAS_MPV` 控制，构建时需要 libmpv 开发文件。
未开启时媒体 API 不参与编译，框架其余部分照常工作。
:::

## 如何融入两层模型

```
+------------------ 应用线程 ------------------+
|  MediaPlayer widget                           |
|    mpv_command(mpv, "loadfile", path)         |
|    播放 / 暂停 / 拖动                          |
+----------------------------------------------+
                     |
                     |  mpv 渲染上下文“需要更新”回调
                     v
+------------------ 渲染线程（持有 GL 上下文）---+
|  mpv_render_context_render() -> 写入 GL FBO     |
|  得到的帧成为一张 GL 纹理                       |
|  Texture widget 像普通命令一样绘制它            |
+------------------------------------------------+
```

1. **mpv 渲染上下文**绑定到渲染线程已持有的同一个 OpenGL 上下文（桌面端即
   GLFW/WGL 上下文）。
2. mpv 异步解码文件，在有新帧时回调“需要更新”，该回调请求渲染线程重绘。
3. 渲染线程调用 `mpv_render_context_render()` 把解码帧画到离屏 GL framebuffer/纹理，
   NeoFlux 随后为该纹理发出普通的“画纹理”渲染命令，视频因此能像其它 widget 一样
   合成（背景、文字叠加、裁剪都生效）。

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
让 `MediaPlayer` 指向它即可端到端验证播放，无需下载外部素材：

```powershell
.\build\bin\<media_demo>.exe --logtostderr --verbose_logging
```

::: tip 用 --logtostderr
mpv 会打印大量诊断信息。加 `--logtostderr` 才能在终端看到（示例默认是 GUI
子系统、无控制台）。
:::

## 依赖要求

- 构建期可用的 libmpv（`mpv/client.h`、`mpv/render_gl.h`）。
- 桌面 GL 路径（`--render_backend=gl`），因为渲染上下文包裹 OpenGL 上下文。
- 你的 mpv 构建所带 ffmpeg/libav 能解码所用封装/编码格式。

## 本工作树中未能核对的内容

`docs/bilingual` 工作树当前并不包含 `MediaPlayer` 源码或 `NEOFLUX_HAS_MPV` 的
CMake 接线。本页记录的是项目规划中的桌面媒体架构；发布产品构建前，请对照主检出里的
媒体源码确认确切的类名与 CMake 选项。
