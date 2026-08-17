# Rendering Pipeline

NeoFlux uses a command-based rendering pipeline. The application layer generates
`RenderCommand` objects, and the render layer consumes and executes them.

## RenderCommand

`RenderCommand` is a flat struct (not a union) that represents a single drawing
operation. It carries a type tag and the payload for that operation:

| Type | Description |
|------|-------------|
| `kNoop` | No operation (padding) |
| `kBeginFrame` | Start a new frame (clear, set viewport) |
| `kEndFrame` | End frame (flush) |
| `kDrawRect` | Draw a filled rectangle |
| `kDrawRoundedRect` | Draw a filled rounded rectangle |
| `kDrawText` | Draw text glyphs |
| `kSave` | Push transform/clip state |
| `kRestore` | Pop transform/clip state |
| `kTranslate` | Apply translation offset |
| `kClipRect` | Set a clip rectangle |

Commands are created via factory functions:

```cpp
RenderCommand cmd = RenderCommand::MakeDrawRect(rect, color);
RenderCommand text_cmd = RenderCommand::MakeDrawText(
    text, position, color, font_size, font_name);
```

## SPSC Ring Queue

Commands are passed from the application layer to the render layer via a
lock-free SPSC ring queue:

```cpp
// Application layer (producer):
render_layer_->Submit(cmd);

// Render layer (consumer):
while (running_) {
  RenderCommand cmd;
  while (queue_.TryPop(cmd)) {
    Execute(cmd);
  }
  WaitForNextFrame();
}
```

The queue capacity is configurable via `--render_queue_capacity` (default 2048).

## Desktop Rendering (OpenGL)

On desktop, the render layer uses OpenGL 3.3 via GLFW. Key components:

### Vertex Buffer

A pre-allocated VBO (64KB) is updated via `glBufferSubData` for each draw call:

```cpp
glBufferData(GL_ARRAY_BUFFER, 64 * 1024, nullptr, GL_DYNAMIC_DRAW);
// ...
glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
```

### Shaders

Vertex shader transforms layout coordinates to NDC:

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

Fragment shader supports solid color and texture (glyph) modes:

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

### Text Rendering

Text is rendered using a glyph texture atlas powered by FreeType:

1. Each glyph is rasterized to a grayscale bitmap.
2. The bitmap is uploaded to a texture atlas.
3. Glyph quad vertices reference atlas UV coordinates.
4. The fragment shader samples the atlas and multiplies by text color.

### Rounded Rectangles

Rounded rectangles are drawn using a triangle fan with sampled corner arcs:

```cpp
// 1 centre + 4 corners * kSeg boundary points
constexpr int kSeg = 10;
float vertices[(1 + 4 * kSeg + 1) * 4];
// ... compute arc points with cos/sin ...
glDrawArrays(GL_TRIANGLE_FAN, 0, vertex_count);
```

## Mobile Rendering (tgfx)

On mobile platforms, NeoFlux uses [tgfx](https://github.com/Tencent/tgfx),
Tencent's 2D graphics library. tgfx provides:

- Unified API across Vulkan, Metal, and OpenGL ES
- GPU-accelerated path rendering
- Text rendering with font subpixel positioning
- Image decoding and filtering

The `TgfxRenderer` class wraps tgfx and translates `RenderCommand`s into tgfx
canvas calls.

## Frame Synchronization

The render thread waits for commands using a condition variable:

```cpp
// Application layer signals new frame:
frame_cv_.notify_one();

// Render thread waits:
std::unique_lock lock(frame_mutex_);
frame_cv_.wait(lock, [this] { return has_commands_ || !running_; });
```

This minimizes idle CPU usage when no rendering is needed.
