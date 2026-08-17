# Font Demo

Demonstrates the font system: default font, explicit font selection, multiple
sizes/colors, and CJK text rendering.

## Run

```bash
./bin/font_demo
```

## Features

- Default font rendering
- Explicit `SetFont()` by name
- Multiple font sizes
- Multiple text colors
- CJK (Chinese/Japanese/Korean) text

## Setup

Place font files in `thirdparty/fonts/`:

```
thirdparty/fonts/
  NotoSansSC-Regular.ttf
  Roboto-Bold.ttf
```

Fonts are registered by filename (without extension).

## Key Code

```cpp
auto default_text = std::make_shared<Text>("Default Font");
default_text->SetFontSize(20.0F);

auto named_text = std::make_shared<Text>("Noto Sans SC");
named_text->SetFont("NotoSansSC-Regular")
          .SetFontSize(20.0F);

auto cjk_text = std::make_shared<Text>(u8"你好世界");
cjk_text->SetFont("NotoSansSC-Regular")
        .SetFontSize(24.0F)
        .SetTextColor({.r = 0, .g = 100, .b = 200, .a = 255});
```

## Notes

- If no font is specified, the first font found in `thirdparty/fonts/` is used.
- For CJK text, ensure the font includes CJK glyphs.
- Font files are excluded from git; distribute them with your application.
