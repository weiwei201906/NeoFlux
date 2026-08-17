# Text

单行文本组件，支持字体大小、颜色、对齐方式，UTF-8 编码。

## 概述

`Text` 继承 `Widget`，用于显示单行文本。使用 FreeType 渲染 TrueType/OpenType 字体。

## 基本用法

```cpp
auto text = std::make_shared<Text>("Hello NeoFlux!");
text->SetFontSize(24.0F)
    .SetTextColor({.r = 26, .g = 26, .b = 38, .a = 255})
    .SetFont("NotoSansSC-Regular")
    .SetAlignment(HAlign::kCenter);
```

## 方法

| 方法 | 说明 |
|------|------|
| `SetText(text)` | 设置文本内容（UTF-8） |
| `GetText() -> std::string_view` | 获取文本内容 |
| `SetFontSize(size)` | 设置字体大小 |
| `SetTextColor(color)` | 设置文本颜色 |
| `SetFont(font_name)` | 设置字体名称（对应配置的字体目录下的文件名，默认 `fonts/`） |
| `SetAlignment(align)` | 设置水平对齐（kLeft / kCenter / kRight） |

## 字体系统

字体文件放在配置的字体目录（默认 `fonts/`，或通过 `SetFontDir()` 设置），通过文件名（不含扩展名）引用。未指定字体时使用第一个被发现的字体。

## 另见

- [字体系统指南](../guide/fonts)
- [字体演示](../examples/font)
