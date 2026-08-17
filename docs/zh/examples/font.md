# 字体演示

字体系统演示：默认字体、显式 `SetFont()` 选择字体、多种字号/颜色、CJK 文本渲染。

## 运行

```bash
./bin/font_demo
```

## 功能

- 默认字体渲染
- 显式字体选择（`SetFont()`）
- 多种字号（12px / 18px / 24px / 32px）
- 多种文本颜色
- CJK（中文/日文/韩文）文本渲染

## 字体放置

将字体文件放入 `thirdparty/fonts/` 目录，通过文件名（不含扩展名）引用：

```cpp
text->SetFont("NotoSansSC-Regular");  // 加载 thirdparty/fonts/NotoSansSC-Regular.ttf
```

## 另见

- [字体系统指南](../guide/fonts)
- [Text API](../api/text)
