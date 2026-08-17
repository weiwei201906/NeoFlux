# Font System

NeoFlux uses FreeType for font rasterization and a glyph texture atlas for
efficient text rendering.

## Adding Fonts

Place font files (`.ttf`, `.otf`, `.ttc`) in the `thirdparty/fonts/` directory:

```
thirdparty/fonts/
  NotoSansSC-Regular.ttf
  Roboto-Bold.ttf
```

The `FontManager` scans this directory at startup and registers fonts by their
filename (without extension).

## Using Fonts

### Default Font

If no font is specified, NeoFlux uses the first font found in
`thirdparty/fonts/`.

```cpp
auto text = std::make_shared<Text>("Hello");  // uses default font
```

### Explicit Font

Select a font by name (filename without extension):

```cpp
auto text = std::make_shared<Text>("Hello");
text->SetFont("NotoSansSC-Regular");
```

### Font Size and Color

```cpp
text->SetFontSize(24.0F);
text->SetTextColor({.r = 255, .g = 0, .b = 0, .a = 255});  // red
```

### Text Alignment

```cpp
text->SetAlignment(HAlign::kCenter);  // within the Text widget's bounds
```

## How It Works

1. **Font loading**: `FontManager` scans `thirdparty/fonts/` and loads each
   font with FreeType.
2. **Glyph rasterization**: When a character is first rendered, FreeType
   rasterizes it to a grayscale bitmap.
3. **Texture atlas**: The bitmap is uploaded to a 1024x1024 texture atlas.
4. **Rendering**: Each glyph is drawn as a textured quad. The fragment shader
   samples the atlas's red channel and multiplies by the text color.

## CJK and Unicode

NeoFlux supports UTF-8 text. The `Text` widget accepts `std::string` (UTF-8)
and renders each Unicode code point. For CJK text, use a font that includes
CJK glyphs (e.g., Noto Sans SC).

```cpp
auto text = std::make_shared<Text>(u8"你好世界");
text->SetFont("NotoSansSC-Regular");
```

## Font Search Paths

`FontManager` searches the following paths (relative to the working directory):

- `thirdparty/fonts/`
- `../thirdparty/fonts/`
- `../../thirdparty/fonts/`

This ensures fonts are found whether the application runs from the project root
or the build output directory.

## Best Practices

- Include only the fonts you need to reduce binary size and memory usage.
- Use a single font family with multiple weights if possible.
- For CJK text, ensure the font includes the necessary glyphs.
- Font files are excluded from git via `.gitignore`; distribute them with your
  application or document where users should place them.
