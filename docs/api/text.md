# Text

Leaf widget that renders a single-line text string.

## Header

```cpp
#include <neoflux/widget/text.h>
```

## Construction

```cpp
auto text = std::make_shared<Text>("Hello NeoFlux");
```

## Methods

All methods return `Text&` for chaining.

### `SetText()`

```cpp
Text& SetText(std::string_view text);
```

Sets the text content (UTF-8).

### `SetFontSize()`

```cpp
Text& SetFontSize(float size);
```

Sets the font size in pixels. Default: 16.

### `SetTextColor()`

```cpp
Text& SetTextColor(const Color& color);
```

Sets the text color.

### `SetFont()`

```cpp
Text& SetFont(std::string_view font_name);
```

Sets the font by name (filename without extension). The font must be in
`thirdparty/fonts/`.

### `SetAlignment()`

```cpp
Text& SetAlignment(HAlign align);
```

Sets horizontal alignment within the widget's bounds.

## Example

```cpp
auto title = std::make_shared<Text>("Welcome");
title->SetFontSize(28.0F)
     .SetTextColor({.r = 33, .g = 33, .b = 33, .a = 255})
     .SetFont("NotoSansSC-Regular")
     .SetAlignment(HAlign::kCenter);
```
