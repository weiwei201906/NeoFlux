# Bundled Fonts

Place TrueType (`.ttf`), OpenType (`.otf`), or TrueType Collection (`.ttc`)
font files in this directory. NeoFlux scans this folder at startup and registers
each font under its filename stem (without extension).

> **Font files are NOT committed to git.** See `.gitignore`. Run the download
> script below to fetch the default CJK font before building examples.

## Download Default Font

```powershell
# Windows
powershell -ExecutionPolicy Bypass -File examples/download_fonts.ps1
```

```bash
# Linux / macOS
bash examples/download_fonts.sh
```

This downloads **Noto Sans SC Regular** (SIL OFL 1.1 license, ~18MB) to
`NotoSansSC-Regular.otf` in this directory.

## Build-Time Copy

CMake automatically copies all fonts from this directory to
`<build>/bin/fonts/` in a POST_BUILD step. Examples reference fonts via
`SetFontDir("fonts")` so they work from the build output directory.

## Usage

Reference fonts by name in your widgets:

```cpp
auto text = std::make_shared<Text>("Hello");
text->SetFont("NotoSansSC-Regular");  // loads NotoSansSC-Regular.otf
```

If `SetFont()` is not called, the first discovered font is used as the default.

## Adding a Font

1. Obtain a `.ttf`, `.otf`, or `.ttc` font file.
2. Place it in this directory.
3. Reference it by filename stem in your code via `SetFont()`.

## Recommended Fonts

| Font | License | Notes |
|------|---------|-------|
| Noto Sans SC | SIL OFL 1.1 | Google's open-source CJK font. Download via script above. |
| Source Han Sans SC | SIL OFL 1.1 | Adobe's open-source CJK font. |
| Hack | MIT | Compact monospace font for Latin text. ~150KB. |
| WenQuanYi Micro Hei | GPLv3 | Compact Chinese font. |
