# 字体系统

NeoFlux 使用字体管理器，在启动时扫描 `thirdparty/fonts/` 目录。

:::warning
如果 `thirdparty/fonts/` 为空或不存在，所有文本 Widget 都会渲染异常（乱码或空白）。发布应用时务必至少附带一个字体文件。渲染中文需包含 CJK 字体（如 NotoSansSC）。
:::

## 支持格式

- TrueType（`.ttf`）
- OpenType（`.otf`）
- TrueType Collection（`.ttc`）

## 使用字体

Widget 通过文件名（不含扩展名）引用字体：

```cpp
auto text = std::make_shared<Text>("Hello World");
text->SetFont("NotoSansSC-Regular");  // 加载 thirdparty/fonts/NotoSansSC-Regular.ttf
```

## 默认字体

若 Widget 未指定字体，则使用第一个被发现的字体作为默认字体。

## 开发者须知

将字体文件放入 `thirdparty/fonts/` 目录即可通过名称引用，无需构建时拷贝。字体文件不纳入 git 版本控制（`.gitignore` 排除）。

## 下一步

- [Text API](../api/text)
- [字体演示](../examples/font)
