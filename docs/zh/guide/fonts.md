# 字体系统

NeoFlux 使用 FreeType 进行字体光栅化，并使用字形纹理图集实现高效文本渲染。

## 配置字体目录

:::danger
必须在调用 `Application::Init()` 之前配置字体目录。如果找不到字体，所有文本 Widget 都会渲染为乱码或空白。
:::

默认情况下，NeoFlux 扫描 `thirdparty/fonts/` 目录查找字体文件。如需使用其他目录，在 `Init()` **之前**调用 `Application::SetFontDir()`：

```cpp
Application app;
app.SetFontDir("assets/fonts");  // 扫描 assets/fonts/ 而非 thirdparty/fonts/
app.Init(argc, argv, 800, 600, "My App");
app.PushRoute("/");
app.Run();
```

`SetFontDir()` 接受相对或绝对路径。相对路径从工作目录解析，并自动向上回退（`../`、`../../`）以适配构建子目录。

## 添加字体

将字体文件（`.ttf`、`.otf`、`.ttc`）放入配置的字体目录：

```
thirdparty/fonts/          （默认，或你自定义的路径）
  NotoSansSC-Regular.ttf
  Roboto-Bold.ttf
```

`FontManager` 在启动时扫描此目录，按文件名（不含扩展名）注册字体。

:::warning
如果配置的字体目录为空或不存在，所有文本 Widget 都会渲染异常（乱码或空白）。发布应用时务必至少附带一个字体文件。渲染中文需包含 CJK 字体（如 NotoSansSC）。
:::

## 支持格式

- TrueType（`.ttf`）
- OpenType（`.otf`）
- TrueType Collection（`.ttc`）

## 使用字体

Widget 通过文件名（不含扩展名）引用字体：

```cpp
auto text = std::make_shared<Text>("Hello World");
text->SetFont("NotoSansSC-Regular");  // 加载配置目录下的 NotoSansSC-Regular.ttf
```

## 默认字体

若 Widget 未指定字体，则使用第一个被发现的字体作为默认字体。

## 字体搜索路径

`FontManager` 搜索配置的目录（默认：`thirdparty/fonts/`），从工作目录开始，然后向上回退：

- `<font_dir>/`
- `../<font_dir>/`
- `../../<font_dir>/`

这确保无论应用从项目根目录还是构建输出目录运行，都能找到字体。通过 `Application::SetFontDir()` 在 `Init()` 之前配置目录。

## 工作原理

1. **字体加载**：`FontManager` 扫描配置的字体目录，用 FreeType 加载每个字体。
2. **字形光栅化**：字符首次渲染时，FreeType 将其光栅化为灰度位图。
3. **纹理图集**：位图上传到 1024x1024 纹理图集。
4. **渲染**：每个字形绘制为带纹理的四边形。片段着色器采样图集的红色通道并乘以文本颜色。

## CJK 与 Unicode

NeoFlux 支持 UTF-8 文本。`Text` Widget 接受 `std::string`（UTF-8）并渲染每个 Unicode 码点。渲染中文需使用包含 CJK 字形的字体（如 Noto Sans SC）。

```cpp
auto text = std::make_shared<Text>(u8"你好世界");
text->SetFont("NotoSansSC-Regular");
```

## CMake：构建时自动拷贝字体

在你自己的工程中，将字体放在 `fonts/` 目录下，通过 CMake 在每次构建时拷贝到可执行文件同级目录：

```cmake
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE neoflux)

add_custom_command(TARGET my_app POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/fonts $<TARGET_FILE_DIR:my_app>/fonts
)
```

然后在代码中配置目录：

```cpp
Application app;
app.SetFontDir("fonts");  // 对应拷贝到输出目录的 fonts/ 文件夹
app.Init(argc, argv, 800, 600, "My App");
```

## 最佳实践

- 仅包含所需字体，减少二进制体积和内存占用。
- 尽可能使用单一字体系列的多种字重。
- 渲染中文时，确保字体包含所需字形。
- 字体文件通过 `.gitignore` 排除在 git 之外；随应用分发或在文档中说明用户应放置的位置。

## 下一步

- [Text API](../api/text)
- [字体演示](../examples/font)
