# Font System

NeoFlux uses FreeType for glyph rasterization and a single grayscale texture
atlas for efficient text rendering. Widgets reference fonts **by name**; the
`FontManager` resolves that name to a file path.

## Supported formats

The scanner (`FontManager::ScanDirectory`) recognizes three TrueType /
OpenType file extensions:

- `.ttf` — TrueType
- `.otf` — OpenType (CFF)
- `.ttc` — TrueType Collection

Each discovered file is registered under its **filename stem** (the file name
without extension). For example `NotoSansSC-Regular.ttf` becomes the font name
`NotoSansSC-regular` (lookups are case-insensitive). The **first** font
scanned becomes the default font used when a widget does not call `SetFont()`.

## The `assets/fonts/` convention

The recommended project layout keeps user-supplied fonts out of source control
next to the executable:

```
my_app/
├── assets/
│   └── fonts/
│       ├── NotoSansSC-Regular.ttf
│       └── Roboto-Bold.ttf
└── ...
```

Add a CMake `POST_BUILD` step that copies the whole `assets/fonts/` tree next
to the produced binary, so the working-directory lookup finds it:

```cmake
add_custom_command(TARGET my_app POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/assets/fonts
          $<TARGET_FILE_DIR:my_app>/assets/fonts)
```

After the build, the layout next to the executable is:

```
build/bin/
├── my_app.exe
└── assets/
    └── fonts/
        ├── NotoSansSC-Regular.ttf
        └── Roboto-Bold.ttf
```

Run the executable from its `bin` directory so the default search path
`./assets/fonts/` resolves:

```powershell
.\build\bin\my_app.exe
```

::: tip Current built-in scan path
In this release the renderer (`TgfxRenderer::InitFonts`) scans `thirdparty/fonts/`
plus `../thirdparty/fonts/` and `../../thirdparty/fonts/` relative fallbacks,
rather than `./assets/fonts/`. Either place your fonts in `thirdparty/fonts/`
for the bundled demos, or use the `assets/fonts/` + `POST_BUILD` convention
above for your own targets and point `FontManager::ScanDirectory("assets/fonts")`
at it. Supported formats and by-stem naming are identical either way.
:::

## Using fonts

### Default font

If a `Text` / `Button` widget does not specify a font, the first discovered
font is used.

```cpp
auto text = std::make_shared<Text>("Hello");  // uses default font
```

### Explicit font

Select a font by its filename stem (case-insensitive):

```cpp
auto text = std::make_shared<Text>("Hello");
text->SetFont("NotoSansSC-Regular");  // loads NotoSansSC-Regular.ttf
```

### Size, color, alignment

```cpp
text->SetFontSize(24.0F);
text->SetTextColor({.r = 255, .g = 0, .b = 0, .a = 255});  // red
text->SetAlignment(HAlign::kCenter);                         // within bounds
```

## How rendering works

1. **FreeType init** — the renderer initializes one `FT_Library` at startup.
2. **Scan** — `FontManager::ScanDirectory(dir)` walks the directory and maps
   lowercase stems to absolute file paths.
3. **Lazy face load** — the first time a glyph is needed, the matching
   `FT_Face` is loaded and cached.
4. **Rasterize + atlas** — glyph bitmaps are uploaded into one grayscale
   texture atlas; each glyph is drawn as a textured quad.
5. **Color** — the fragment shader samples the atlas alpha channel and
   multiplies by the widget's text color.

::: warning No fonts = no text
If the scan directory does not exist or contains no font files, the renderer
logs `No fonts found ... Text rendering will be disabled` and draws no text.
Always ship at least one font.
:::

## CJK and Unicode

`Text` accepts a UTF-8 `std::string`. For CJK content use a font that covers
the required glyphs (e.g. Noto Sans SC):

```cpp
auto text = std::make_shared<Text>(u8"你好世界");
text->SetFont("NotoSansSC-Regular");
```

## Best practices

- Ship only the weights/subsets you need to keep memory and download size down.
- Reference fonts by stable stem name; renaming a file renames the font.
- Keep `assets/fonts/` out of git (large binaries) and copy it at build time.
