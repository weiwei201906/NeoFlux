# 渲染管线

NeoFlux 使用基于命令的渲染管线。Application 层生成 `RenderCommand` 对象，Render 层消费并执行它们。

## 两层架构

- **Application 层**：构建 Widget 树，Taitank 计算布局，录制 RenderCommand
- **Render 层**：从 SPSC 环形队列消费命令，tgfx 执行绘制

## RenderCommand

`RenderCommand` 是扁平结构体（非 union），携带类型标签和对应操作的负载：

| 类型 | 说明 |
|------|------|
| `kNoop` | 无操作（填充） |
| `kBeginFrame` | 开始新帧（清屏、设置视口） |
| `kEndFrame` | 结束帧（刷新） |
| `kDrawRect` | 绘制填充矩形 |
| `kDrawRoundedRect` | 绘制填充圆角矩形 |
| `kDrawText` | 绘制文本字形 |
| `kSave` | 压入变换/裁剪状态 |
| `kRestore` | 弹出变换/裁剪状态 |
| `kTranslate` | 应用平移偏移 |
| `kClipRect` | 设置裁剪矩形 |

通过工厂函数创建命令：

```cpp
RenderCommand cmd = RenderCommand::MakeDrawRect(rect, color);
RenderCommand text_cmd = RenderCommand::MakeDrawText(
    text, position, color, font_size, font_name);
```

## SPSC 环形队列

命令通过无锁 SPSC 环形队列从 Application 层传递到 Render 层：

```cpp
// Application 层（生产者）：
render_layer_->Submit(cmd);

// Render 层（消费者）：
while (running_) {
  RenderCommand cmd;
  while (queue_.TryPop(cmd)) {
    Execute(cmd);
  }
  WaitForNextFrame();
}
```

队列容量通过 `--render_queue_capacity` 配置（默认 2048），自动向上取整为 2 的幂。

## 桌面端渲染（OpenGL）

桌面端通过 GLFW 创建窗口与 OpenGL 3.3 上下文。

### 顶点缓冲区

预分配 64KB VBO，每次绘制调用通过 `glBufferSubData` 更新：

```cpp
glBufferData(GL_ARRAY_BUFFER, 64 * 1024, nullptr, GL_DYNAMIC_DRAW);
// ...
glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
```

### 着色器

顶点着色器将布局坐标转换为 NDC：

```glsl
layout(location=0) in vec4 a_pos;  // x, y, u, v
uniform vec2 u_resolution;
uniform vec2 u_translate;
out vec2 v_uv;

void main() {
  vec2 p = a_pos.xy + u_translate;
  vec2 clip = (p / u_resolution) * 2.0 - 1.0;
  gl_Position = vec4(clip.x, -clip.y, 0.0, 1.0);
  v_uv = a_pos.zw;
}
```

片段着色器支持纯色和纹理（字形）模式：

```glsl
uniform vec4 u_color;
uniform sampler2D u_texture;
uniform int u_use_texture;
in vec2 v_uv;

void main() {
  if (u_use_texture != 0) {
    float a = texture(u_texture, v_uv).r;
    frag_color = vec4(u_color.rgb, u_color.a * a);
  } else {
    frag_color = u_color;
  }
}
```

### 文本渲染

文本使用 FreeType 驱动的字形纹理图集渲染：

1. 每个字形光栅化为灰度位图
2. 位图上传到纹理图集
3. 字形四边形顶点引用图集 UV 坐标
4. 片段着色器采样图集并乘以文本颜色

### 圆角矩形

圆角矩形使用三角形扇带采样角弧绘制：

```cpp
// 1 中心 + 4 角 * kSeg 边界点
constexpr int kSeg = 10;
float vertices[(1 + 4 * kSeg + 1) * 4];
// ... 用 cos/sin 计算弧点 ...
glDrawArrays(GL_TRIANGLE_FAN, 0, vertex_count);
```

## 移动端渲染（tgfx）

移动端使用 [tgfx](https://github.com/Tencent/tgfx)，腾讯的 2D 图形库。tgfx 提供：

- 跨 Vulkan、Metal、OpenGL ES 的统一 API
- GPU 加速路径渲染
- 支持字体亚像素定位的文本渲染
- 图像解码与滤波

`TgfxRenderer` 类封装 tgfx，将 `RenderCommand` 翻译为 tgfx canvas 调用。

## 帧同步

渲染线程使用条件变量等待命令：

```cpp
// Application 层信号新帧：
frame_cv_.notify_one();

// Render 线程等待：
std::unique_lock lock(frame_mutex_);
frame_cv_.wait(lock, [this] { return frame_ready_ || !running_; });
```

这在无需渲染时将空闲 CPU 占用降至零。

## 下一步

- [架构](./architecture)
- [跨平台](./cross-platform)
