# 字体系统

NeoFlux 用 FreeType 做字形光栅化，并用一张灰度纹理图集高效渲染文字。
Widget 按**名字**引用字体，`FontManager` 把名字解析为文件路径。

## 支持格式

扫描器（`FontManager::ScanDirectory`）识别三种字体格式：

- `.ttf` —— TrueType
- `.otf` —— OpenType（CFF）
- `.ttc` —— TrueType Collection

每个被发现的文件按其**文件名主干**（去掉扩展名）登记。例如
`NotoSansSC-Regular.ttf` 注册为字体名 `NotoSansSC-regular`（查找时忽略大小写）。
最先扫描到的字体成为默认字体，widget 未调用 `SetFont()` 时使用它。

## `assets/fonts/` 约定

推荐的工程布局把用户字体放在可执行文件旁边，不纳入源码仓库：

```
my_app/
├── assets/
│   └── fonts/
│       ├── NotoSansSC-Regular.ttf
│       └── Roboto-Bold.ttf
└── ...
```

在 CMake 里加一个 `POST_BUILD` 步骤，把整个 `assets/fonts/` 目录拷贝到产物二进制旁，
这样运行时按工作目录查找就能命中：

```cmake
add_custom_command(TARGET my_app POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/assets/fonts
          $<TARGET_FILE_DIR:my_app>/assets/fonts)
```

构建后，二进制旁的布局为：

```
build/bin/
├── my_app.exe
└── assets/
    └── fonts/
        ├── NotoSansSC-Regular.ttf
        └── Roboto-Bold.ttf
```

从 `bin` 目录运行可执行文件，默认查找路径 `./assets/fonts/` 即可命中：

```powershell
.\build\bin\my_app.exe
```

::: tip 当前内置扫描路径
本版本中，渲染器（`TgfxRenderer::InitFonts`）扫描的是 `thirdparty/fonts/`，
并附带 `../thirdparty/fonts/`、`../../thirdparty/fonts/` 相对回退，而非 `./assets/fonts/`。
自带示例把字体放进 `thirdparty/fonts/`；你自己的目标则可采用上面的
`assets/fonts/` + `POST_BUILD` 约定，并显式调用
`FontManager::ScanDirectory("assets/fonts")`。两种方式支持的格式与按主干命名的规则完全一致。
:::

## 使用字体

### 默认字体

`Text`/`Button` 未指定字体时，使用第一个发现的字体。

```cpp
auto text = std::make_shared<Text>("Hello");  // 使用默认字体
```

### 指定字体

按文件名主干选择（忽略大小写）：

```cpp
auto text = std::make_shared<Text>("Hello");
text->SetFont("NotoSansSC-Regular");  // 加载 NotoSansSC-Regular.ttf
```

### 字号、颜色、对齐

```cpp
text->SetFontSize(24.0F);
text->SetTextColor({.r = 255, .g = 0, .b = 0, .a = 255});  // 红色
text->SetAlignment(HAlign::kCenter);                         // 在 bounds 内居中
```

## 渲染流程

1. **初始化 FreeType** —— 启动时建立一个 `FT_Library`。
2. **扫描** —— `FontManager::ScanDirectory(dir)` 遍历目录，把小写主干映射为绝对路径。
3. **按需加载 face** —— 首次需要某字形时才加载并缓存对应的 `FT_Face`。
4. **光栅化 + 图集** —— 字形位图上传到一张灰度纹理图集，每个字形画成纹理四边形。
5. **着色** —— 片元着色器采样图集 alpha 通道，再乘以 widget 的文字颜色。

::: warning 没有字体就没有文字
若扫描目录不存在或没有字体文件，渲染器会记录
`No fonts found ... Text rendering will be disabled`，并不绘制任何文字。
务必至少附带一个字体。
:::

## CJK 与 Unicode

`Text` 接受 UTF-8 `std::string`。中文内容请使用覆盖相应字形的字体（如 Noto Sans SC）：

```cpp
auto text = std::make_shared<Text>(u8"你好世界");
text->SetFont("NotoSansSC-Regular");
```

## 最佳实践

- 只附带所需字重/子集，控制内存与下载体积。
- 用稳定的主干名引用字体；重命名文件即等于重命名字体。
- 把 `assets/fonts/` 排除在 git 之外（大体积二进制），构建时再拷贝。
