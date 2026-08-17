# RenderCommand

Tagged union representing a single rendering operation. Passed from the
application layer to the render layer via the SPSC ring queue.

## Header

```cpp
#include <neoflux/render/render_command.h>
```

## Command Types

```cpp
enum class RenderCommandType : uint8_t {
  kBeginFrame,
  kEndFrame,
  kDrawRect,
  kDrawRoundedRect,
  kDrawText,
  kTranslate,
  kClipRect,
  kRestore,
};
```

## Factory Functions

### `MakeBeginFrame()`

```cpp
static RenderCommand MakeBeginFrame(const Color& clear_color);
```

Starts a new frame. Clears the screen with the given color.

### `MakeEndFrame()`

```cpp
static RenderCommand MakeEndFrame();
```

Ends the frame. Swaps buffers.

### `MakeDrawRect()`

```cpp
static RenderCommand MakeDrawRect(const Rect& rect, const Color& color);
```

Draws a filled rectangle.

### `MakeDrawRoundedRect()`

```cpp
static RenderCommand MakeDrawRoundedRect(const Rect& rect, const Color& color,
                                         float corner_radius);
```

Draws a filled rounded rectangle.

### `MakeDrawText()`

```cpp
static RenderCommand MakeDrawText(float x, float y, std::string text,
                                  std::string font_name, float font_size,
                                  const Color& color);
```

Draws text at the given position.

### `MakeTranslate()`

```cpp
static RenderCommand MakeTranslate(float dx, float dy);
```

Pushes a translation transform.

### `MakeClipRect()`

```cpp
static RenderCommand MakeClipRect(const Rect& rect);
```

Sets a clip rectangle.

### `MakeRestore()`

```cpp
static RenderCommand MakeRestore();
```

Pops the transform/clip state.

## Fields

```cpp
RenderCommandType type;
Rect rect;
Color color;
float corner_radius;
float x, y;
float dx, dy;
float font_size;
std::string text;
std::string font_name;
```

Only fields relevant to the command type are valid.

## Notes

- `RenderCommand` uses `std::string` for text and font names, so it is not
  trivially copyable. The SPSC ring queue uses `std::construct_at` /
  `std::destroy_at` for safe construction.
- Commands are consumed in FIFO order by the render layer.
