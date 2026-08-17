# RenderContext

Interface for generating render commands during the paint phase. Widgets call
methods on `RenderContext` to draw themselves.

## Header

```cpp
#include <neoflux/render/render_context.h>
```

## Methods

### `DrawRect()`

```cpp
void DrawRect(const Rect& rect, const Color& color);
```

Draws a filled rectangle.

### `DrawRoundedRect()`

```cpp
void DrawRoundedRect(const Rect& rect, const Color& color, float radius);
```

Draws a filled rounded rectangle with the given corner radius.

### `DrawText()`

```cpp
void DrawText(float x, float y, std::string_view text,
              const Font& font, float font_size, const Color& color);
```

Draws text at the given position.

### `Save()` / `Restore()`

```cpp
void Save();
void Restore();
```

Pushes/pops the transform and clip state stack.

### `Translate()`

```cpp
void Translate(float dx, float dy);
```

Applies a translation offset to subsequent draw calls.

### `ClipRect()`

```cpp
void ClipRect(const Rect& rect);
```

Sets a clip rectangle (intersected with the current clip).

## Usage in Widget::Paint

```cpp
void MyWidget::Paint(RenderContext& context) {
  context.Save();
  context.Translate(bounds_.x, bounds_.y);
  context.ClipRect({0, 0, bounds_.width, bounds_.height});

  context.DrawRoundedRect(
      {0, 0, bounds_.width, bounds_.height},
      background_color_,
      border_radius_);

  context.DrawText(padding_, padding_, text_, font_, font_size_, text_color_);

  context.Restore();
}
```

## Notes

- `RenderContext` converts method calls into `RenderCommand` objects and submits
  them to the render layer.
- Transform and clip state is managed via a stack (`Save`/`Restore`).
- All coordinates are in layout space (pixels, top-left origin).
