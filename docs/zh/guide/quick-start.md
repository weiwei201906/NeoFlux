# 快速开始

本指南带你从克隆仓库开始，在 Windows 上使用 MSVC 工具链与 Ninja 生成器，
把 NeoFlux 跑起来。

## 环境要求

- **CMake 3.20 或更新**
- **MSVC 2022**（Visual Studio 17），安装“使用 C++ 的桌面开发”工作负载
- **Ninja** 构建工具（Visual Studio 自带，或单独安装）
- 支持 C++20 的编译器（MSVC 19.3x+）

第三方依赖（`gflags`、`glog`、`Taitank`、`GLFW`、`FreeType`，以及可选的
`tgfx`）都通过 `add_subdirectory(thirdparty)` 引入，无需手动安装。

::: tip Windows 上的 OpenGL
桌面版通过 GLFW/WGL 创建 GL 上下文，任何支持 OpenGL 2.1+ 的现代显卡驱动都可以。
:::

## 用 MSVC + Ninja 构建

打开 **x64 Native Tools Command Prompt for VS 2022**（或在普通终端里运行
`vcvars64.bat`），确保 `cl.exe`、`link.exe`、`ninja` 在 `PATH` 中，然后在仓库根目录
配置并构建：

```powershell
# 配置（out-of-source 构建）
cmake -S . -B build -G Ninja

# 构建静态库与全部示例
cmake --build build
```

构建产物位于 `build\bin\`：

| 路径                          | 内容                  |
|-------------------------------|-----------------------|
| `build\bin\hello_neoflux.exe` | Hello World 示例      |
| `build\bin\counter.exe`       | 计数器示例            |
| `build\bin\flex_demo.exe`     | Flex 布局演示         |
| `build\bin\font_demo.exe`     | 字体渲染演示          |
| `build\bin\scroll_demo.exe`   | ScrollView 演示        |
| `build\bin\loading_demo.exe`  | 协程状态机演示        |
| `build\bin\drag_demo.exe`     | 拖拽 + 长按演示       |
| `build\lib\`                  | `neoflux` 静态库      |

如需同时构建单元测试：

```powershell
cmake -S . -B build -G Ninja -DNEOFLUX_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

::: warning GUI 子系统，没有控制台
示例以 `CMAKE_WIN32_EXECUTABLE ON` 构建，因此在 Windows 上启动时**不会**弹出控制台窗口。
默认日志写入 `./logs/` 文件而非终端，调试时请使用下面的日志参数。
:::

## 运行示例

从仓库根目录运行（这样相对路径 `thirdparty/fonts` 和 `./logs` 才能正确解析）：

```powershell
.\build\bin\hello_neoflux.exe
```

标题为 *NeoFlux - Hello World* 的窗口会打开。你会看到标题、实时计数器，以及按钮：
一个让计数加一，另一个压入第二个路由，可再返回。

## 运行参数（gflags）

所有参数均为可选，跟在可执行文件名之后：

```powershell
.\build\bin\hello_neoflux.exe --render_backend=gl --target_fps=120 --logtostderr --verbose_logging
```

| 参数 | 示例值 | 作用 |
|------|--------|------|
| `--render_backend` | `gl` | 选择渲染后端，可选 `vulkan`（默认）、`gl`、`cpu`；未实现的后端会回退到 OpenGL 并给出警告。 |
| `--target_fps` | `120` | 限制应用事件循环的帧率。 |
| `--render_queue_capacity` | `4096` | SPSC 渲染命令环形队列容量（内部向上取整为 2 的幂）。 |
| `--verbose_logging` | （出现即开） | 开启 `VLOG(1)` 调试输出。 |
| `--logtostderr` | （出现即开） | 日志全部输出到 stderr 而非文件。 |
| `--log_dir=路径` | `--log_dir=./logs` | `.log` 文件目录（默认 `./logs`）。 |

::: tip 日常调试命令
日常开发时，建议这样运行，把日志打到终端：

```powershell
.\build\bin\hello_neoflux.exe --logtostderr --verbose_logging
```

完整参数说明见[配置](./configuration)页面。
:::

## 最小应用

每个 NeoFlux 程序结构都一样：注册路由、创建 `Application`、压入初始路由、
然后运行阻塞式事件循环。

```cpp
#include <neoflux/app/application.h>
#include <neoflux/widget/button.h>
#include <neoflux/widget/container.h>
#include <neoflux/widget/route_registry.h>
#include <neoflux/widget/text.h>
#include <neoflux/widget/widget.h>

using namespace neoflux;

std::shared_ptr<Widget> BuildHomePage(BuildContext& /*ctx*/) {
  auto root = std::make_shared<Container>();
  root->SetFlexDirection(FlexDirection::kColumn)
      .SetJustifyContent(HAlign::kCenter)
      .SetAlignItems(VAlign::kCenter)
      .SetBackgroundColor({.r = 245, .g = 245, .b = 245, .a = 255})
      .SetPadding({.left = 24, .top = 24, .right = 24, .bottom = 24});

  auto title = std::make_shared<Text>("Hello NeoFlux");
  title->SetFontSize(28.0F).SetTextColor({.r = 33, .g = 33, .b = 33, .a = 255});
  root->AddChild(title);

  auto button = std::make_shared<Button>("Click Me");
  button->SetOnPressed([]() { /* 处理点击 */ });
  root->AddChild(button);
  return root;
}

int main(int argc, char** argv) {
  RouteRegistry::Instance().RegisterRoute("/", BuildHomePage);

  Application app;
  if (!app.Init(argc, argv, 480, 360, "My First NeoFlux App")) {
    return 1;
  }
  app.PushRoute("/");
  app.Run();
  return 0;
}
```

链接 `neoflux` 静态目标即可（若直接使用日志/参数，再链接 `glog::glog` 和
`gflags`）。

## 下一步

- 阅读[架构](./architecture)，理解两层设计与命令如何跨线程传递。
- 跟随[示例](../examples/hello)查看真实 widget 代码。
- 浏览[Widget 指南](./widgets)了解完整组件集。
