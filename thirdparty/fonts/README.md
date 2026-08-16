# Bundled Fonts

Place TrueType (`.ttf`), OpenType (`.otf`), or TrueType Collection (`.ttc`)
font files in this directory to bundle them with NeoFlux.

The renderer searches for fonts in the following priority order:

1. `--font_path` command-line flag
2. `thirdparty/fonts/` (this directory)
3. Platform system font directories

## Recommended Fonts

For CJK (Chinese/Japanese/Korean) support, use one of these open-source fonts:

| Font | License | Notes |
|------|---------|-------|
| Noto Sans SC | OFL-1.1 | Google's open-source CJK font. Large (~10MB). |
| Source Han Sans SC | OFL-1.1 | Adobe's open-source CJK font. |
| WenQuanYi Micro Hei | GPLv3 | Compact Chinese font. |

For Latin-only use, DejaVu Sans or any system font works out of the box.

## Adding a Font

1. Download a `.ttf` or `.otf` font file.
2. Place it in this directory.
3. (Optional) Rename it to match the expected filename in the renderer's
   search list, or pass `--font_path=thirdparty/fonts/yourfont.ttf`.

## CMake Installation

Fonts in this directory are automatically copied to the build output's
`fonts/` subdirectory by the build system.
