# Font System

NeoFlux uses FreeType for font rasterization and a glyph texture atlas for
efficient text rendering.

## Configuring the Font Directory

:::danger
You must configure the font directory before calling `Application::Init()`.
If no fonts are found, all text widgets will render as garbled or blank.
:::

By default, NeoFlux scans `fonts/` for font files. To use a
different directory, call `Application::SetFontDir()` **before** `Init()`:

```cpp
Application app;
app.SetFontDir("assets/fonts");  // scan assets/fonts/ instead of fonts/
app.Init(argc, argv, 800, 600, "My App");
app.PushRoute("/");
app.Run();
```

`SetFontDir()` accepts a relative or absolute path. Relative paths are resolved
from the working directory, with automatic upward fallback (`../`, `../../`) to
handle build subdirectories.

## Adding Fonts

Place font files (`.ttf`, `.otf`, `.ttc`) in your configured font directory:

```
fonts/                    (default, or your custom path)
  NotoSansSC-Regular.ttf
  Roboto-Bold.ttf
```

The `FontManager` scans this directory at startup and registers fonts by their
filename (without extension).

:::warning
If the configured font directory is empty or missing, all text widgets will
render incorrectly (garbled or blank). Ship at least one font file with your
application. For CJK text, include a CJK-capable font such as NotoSansSC.
:::

## Using Fonts

### Default Font

If no font is specified, NeoFlux uses the first font found in
`fonts/`.

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

1. **Font loading**: `FontManager` scans the configured font directory and
   loads each font with FreeType.
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

`FontManager` searches the configured directory (default: `fonts/`)
relative to the working directory, then falls back upward:

- `<font_dir>/`
- `../<font_dir>/`
- `../../<font_dir>/`

This ensures fonts are found whether the application runs from the project root
or the build output directory. Configure the directory via
`Application::SetFontDir()` before `Init()`.

## CMake: Auto-Copy Fonts at Build Time

In your own project, place fonts in a `fonts/` directory and use CMake to copy
them next to the executable on every build:

```cmake
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE neoflux)

add_custom_command(TARGET my_app POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/fonts $<TARGET_FILE_DIR:my_app>/fonts
)
```

Then configure the directory in code:

```cpp
Application app;
app.SetFontDir("fonts");  // matches the copied fonts/ folder
app.Init(argc, argv, 800, 600, "My App");
```

## Best Practices

- Include only the fonts you need to reduce binary size and memory usage.
- Use a single font family with multiple weights if possible.
- For CJK text, ensure the font includes the necessary glyphs.
- Font files are excluded from git via `.gitignore`; distribute them with your
  application or document where users should place them.
