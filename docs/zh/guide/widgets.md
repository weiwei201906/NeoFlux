# Widget 系统

NeoFlux 采用类 Flutter 的 Widget 开发模型。每个 UI 元素都是一个 `Widget`，可以包含子组件。

## 核心 Widget

| Widget | 说明 |
|--------|------|
| `Widget` | 抽象基类，重写 `Build()`、`OnMeasure()`、`Paint()` 及事件回调 |
| `Container` | Flexbox 容器，支持 padding、margin、背景色、圆角、flex direction |
| `Text` | 单行文本，支持字体大小、颜色、对齐方式，UTF-8 编码 |
| `Button` | 可点击按钮，支持标签、按下回调、按下状态样式 |
| `ScrollView` | 可滚动视口，支持滚轮与拖拽滚动，内容裁剪 |
| `Draggable` | 可拖拽容器，绘制时平移不影响布局 |
| `Expanded` | 设置了 flex_grow 的容器，填充父容器剩余空间 |
| `SizedBox` | 固定宽高的容器，用于固定间距 |

## 自定义 Widget

继承 `Widget` 并重写虚函数：

### 核心虚函数

| 函数 | 用途 |
|------|------|
| `Build(BuildContext&)` | 为有状态组件返回子 Widget |
| `OnMeasure(width, mode, height, mode)` | 叶子组件返回固有尺寸 |
| `Paint(RenderContext&)` | 生成渲染命令 |
| `HitTest(Point)` | 测试点是否命中此 Widget |
| `OnPointerDown(const Point&)` | 处理按下事件（返回 true 表示消费） |
| `OnPointerUp(const Point&)` | 处理释放事件 |
| `OnPointerMove(const Point&)` | 处理指针移动（悬停/拖拽） |
| `OnPointerEnter()` | 指针进入 Widget 边界 |
| `OnPointerExit()` | 指针离开 Widget 边界 |
| `OnPointerScroll(const Point&, x, y)` | 处理滚动事件 |
| `GetWidgetName()` | 返回 Widget 名称（string_view，用于调试） |
| `GetPaintOffset()` | 返回绘制时的变换偏移（默认零） |

:::tip
`GetPaintOffset()` 是虚函数。在绘制时应用变换的 Widget（例如带拖拽偏移的 `Draggable`）**必须**重写它，这样事件分发才能正确地将视觉坐标转换为 Widget 局部坐标。否则，点击 Widget 的视觉位置会命中失败，因为 `HitTest` 和 `local_pos` 计算使用的是布局坐标。
:::

## 绘制时变换与命中测试

Widget 可以在绘制时应用变换（例如 `Draggable` 按 `drag_offset_` 平移）。这些变换**不**影响 Taitank 布局（`bounds_` 保持在布局位置）。为了保持命中测试和事件坐标与视觉位置一致：

1. **`GetPaintOffset()`** 返回视觉偏移（在子类中重写）。
2. **`HitTest(Point)`** 在委托给基类之前减去绘制偏移，将视觉点击坐标转换为布局坐标。
3. **事件分发**（`DispatchPointerEvent`/`DispatchPointerMove`）在计算 `local_pos` 时减去 `GetPaintOffset()`，使处理函数收到相对于视觉左上角的坐标。

```
视觉位置 = 布局位置 + GetPaintOffset()
local_pos = 鼠标全局位置 - 视觉位置
```

## Draggable

`Draggable` 是一个容器，其子组件在拖拽时跟随指针。Widget **中心始终跟随光标**——按下时立即跳转到以点击点为中心，移动过程中保持居中。

```cpp
auto drag = std::make_shared<Draggable>();
drag->AddChild(box);  // 任意 Widget 树
parent->AddChild(drag);
```

**工作原理：**
- `OnPointerDown`：设置 `dragging_ = true`，立即调整 `drag_offset_` 使 Widget 中心落在光标上。
- `OnPointerMove`：`drag_offset_ += local_pos - bounds.size / 2`，保持中心在光标上。不调用 `MarkNeedsBuild()`——偏移仅在绘制时应用，因此只需要事件分发调用的 `MarkFrameDirty()`。
- `OnPointerUp`：结束拖拽，返回 `kIdle` 状态。
- `Paint`：绘制子组件前 `context.Translate(drag_offset_)`。
- `GetPaintOffset`：返回 `drag_offset_` 以正确命中测试。
- `HitTest`：重写以在基类命中测试前减去 `drag_offset_`。

:::warning
`Draggable` 使用绘制时平移，而非布局更改。拖拽期间 Widget 的 `bounds_`（布局位置）永远不变。这意味着兄弟 Widget 不会重排，拖拽每帧是 $O(1)$——不需要 Taitank 重新布局。
:::

```cpp
class MyWidget : public Widget {
 public:
  std::shared_ptr<Widget> Build(BuildContext& ctx) override {
    auto container = std::make_shared<Container>();
    // 构建子组件树
    return container;
  }

  void Paint(RenderContext& ctx) override {
    // 自定义绘制
    Widget::Paint(ctx);
  }
};
```

## 状态管理

- `StatelessWidget`：无状态组件基类
- `StatefulWidget`：有状态组件基类，配合 `State<W>` 使用
- `SetState()`：标记状态变化，触发重建

## TextField（文本输入框）

单行可编辑文本输入组件，支持光标导航、占位符文本、UTF-8 插入/删除和焦点管理。

```cpp
auto field = std::make_shared<TextField>();
field->SetPlaceholder("请输入姓名...");
field->SetFontSize(16.0F);
field->SetOnSubmit([](std::string_view text) {
  LOG(INFO) << "提交: " << text;
});
container->AddChild(field);
```

### 键盘导航

| 按键 | 行为 |
|------|------|
| 左/右方向键 | 光标移动一个字符 |
| Home/End | 光标移到开头/末尾 |
| Backspace | 删除光标前字符 |
| Delete | 删除光标后字符 |
| Enter | 触发 `OnSubmit` 回调 |
| Ctrl+A | 光标移到末尾（全选） |

点击输入框获取键盘焦点，光标以约 1Hz 频率闪烁。TextField 完全支持 UTF-8：多字节字符在光标移动和删除时被视为单个单元。

:::tip
TextField 必须在构造函数中调用 `EnableMeasureFunction()`，这样 Taitank 才能知道其固有尺寸。否则组件尺寸为零，命中测试会失败（点击无反应）。
:::

## MediaWidget（媒体播放）

基于 **ffplay** 子进程的媒体播放组件（来自 FFmpeg）。以子进程方式启动 ffplay 进行音视频解码和渲染。这保持了框架的轻量性——不链接任何 FFmpeg 库，相同代码可在 Windows、Linux 和 macOS 上运行。

```cpp
auto media = std::make_shared<MediaWidget>();
media->SetSource("video.mp4");
media->SetExtraArgs("-vcodec h264 -acodec aac -fs");
media->Play();
container->AddChild(media);
```

### FFplay 配置

- **默认路径**：`"ffplay"`（运行时从 `PATH` 解析）
- **编译期路径**：`-DNEOFLUX_FFPLAY_PATH=/usr/bin/ffplay`
- **运行期路径**：`media->SetFfplayPath("/custom/ffplay")`
- **额外参数**：`media->SetExtraArgs("-fs -autoexit")` — 追加在默认 `-autoexit` 之后、源 URL 之前

### 环境要求

ffplay 必须已安装并在 `PATH` 中（或通过 `NEOFLUX_FFPLAY_PATH` 配置）：

- **Windows**：从 [gyan.dev](https://www.gyan.dev/ffmpeg/builds/) 下载，将 `bin/` 加入 `PATH`
- **Linux**：`sudo apt install ffmpeg`
- **macOS**：`brew install ffmpeg`

:::warning
MediaWidget 使用子进程隔离，因此 FFmpeg 的 GPL/LGPL 许可证不会传播到 NeoFlux 框架二进制文件。框架本身保持 GPL-3.0。
:::

### 测试视频

生成 10 秒带音频的测试图案视频：

```bash
# Linux/macOS
bash examples/media_demo/generate_test_video.sh

# Windows
examples\media_demo\generate_test_video.bat
```

这会使用 ffmpeg 的 `testsrc` 和 `sine` 滤镜生成 `examples/media_demo/test_video.mp4`。

## 下一步

- [Flex 布局](./layout)
- [输入与事件](./input)
