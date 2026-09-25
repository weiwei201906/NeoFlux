# Bundled Fonts

Place TrueType (`.ttf`), OpenType (`.otf`), or TrueType Collection (`.ttc`)
font files in this directory. NeoFlux scans this folder at startup and registers
each font under its filename stem (without extension).

> **Font files are NOT committed to git.** See `.gitignore`. Place your own
> fonts here before building; the repository does not ship font binaries.

## Download Default Font

A default CJK font (**Noto Sans SC Regular**, SIL OFL 1.1 license) can be
downloaded manually from the Noto CJK repository:

```powershell
# Windows PowerShell
Invoke-WebRequest -Uri "https://github.com/notofonts/noto-cjk/raw/main/Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf" -OutFile "fonts\NotoSansCJKsc-Regular.otf"
```

```bash
# Linux / macOS
curl -L -o fonts/NotoSansCJKsc-Regular.otf \
  https://github.com/notofonts/noto-cjk/raw/main/Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf
```

## Build-Time Copy

CMake automatically copies all fonts from this directory to
`<build>/bin/assets/fonts/` in a POST_BUILD step. The quick-start application
uses `SetFontDir("./assets/fonts/")` (relative to the exe working directory).

## Usage

Reference fonts by name in your widgets:

```cpp
auto text = std::make_shared<Text>("Hello");
text->SetFont("NotoSansCJKsc-Regular");  // loads NotoSansCJKsc-Regular.otf
```

If `SetFont()` is not called, the first discovered font is used as the default.

## Adding a Font

1. Obtain a `.ttf`, `.otf`, or `.ttc` font file.
2. Place it in this directory.
3. Reference it by filename stem in your code via `SetFont()`.

## Recommended Fonts

| Font | License | Notes |
|------|---------|-------|
| Noto Sans SC | SIL OFL 1.1 | Google's open-source CJK font. Download via commands above. |
| Source Han Sans SC | SIL OFL 1.1 | Adobe's open-source CJK font. |
| Hack | MIT | Compact monospace font for Latin text. ~150KB. |
| WenQuanYi Micro Hei | GPLv3 | Compact Chinese font. |
