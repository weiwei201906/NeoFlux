# Bundled Fonts

Place TrueType (`.ttf`), OpenType (`.otf`), or TrueType Collection (`.ttc`)
font files in this directory. NeoFlux scans this folder at startup and registers
each font under its filename stem (without extension).

## Usage

Reference fonts by name in your widgets:

```cpp
auto text = std::make_shared<Text>("Hello");
text->SetFont("NotoSansSC-Regular");  // loads NotoSansSC-Regular.ttf
```

If `SetFont()` is not called, the first discovered font is used as the default.

## Adding a Font

1. Obtain a `.ttf`, `.otf`, or `.ttc` font file.
2. Place it in this directory.
3. Reference it by filename stem in your code via `SetFont()`.

No build-time copying is required — the renderer loads fonts directly from
this directory at runtime.

## Recommended Fonts

For CJK (Chinese/Japanese/Korean) support, use one of these open-source fonts:

| Font | License | Notes |
|------|---------|-------|
| Noto Sans SC | OFL-1.1 | Google's open-source CJK font. |
| Source Han Sans SC | OFL-1.1 | Adobe's open-source CJK font. |
| WenQuanYi Micro Hei | GPLv3 | Compact Chinese font. |
