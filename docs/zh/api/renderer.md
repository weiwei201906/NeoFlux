# 渲染器 / 渲染后端

渲染层运行在独立线程上，通过 SPSC 环形队列消费应用线程推入的渲染命令，并把命令
重放为 tgfx 绘制调用：所有平台统一由 tgfx `Window` 持有 GPU 上下文与交换链，
`context->submit()` 完成每帧呈现。具体 GPU 后端由 tgfx 自己的 `TGFX_USE_*`
编译期开关决定——没有运行时后端参数（见 [GPU 后端](../guide/configuration.md)）。

## RenderLayer

```cpp
class RenderLayer;  // 通过 Application::GetRenderLayer() 获取
```

通常不要直接构造。`Application` 持有它并通过以下方式暴露：

```cpp
RenderLayer& Application::GetRenderLayer() noexcept;
```

它（经 `TgfxRenderer`）持有 tgfx 设备、命令接收端，并负责每帧呈现。

## GlfwBridge（桌面窗口与输入）

```cpp
class GlfwBridge : public NonCopyable;
```

头文件：`<neoflux/renderers/glfw_bridge.h>`。封装 GLFW 窗口（以 `GLFW_NO_API`
创建）并把系统输入翻译成 NeoFlux 回调。它是纯窗口 + 输入桥：GPU 上下文、表面与
呈现都归 `TgfxRenderer` 内的 tgfx `Window`。

| 方法 | 签名 | 说明 |
|------|------|------|
| `Init` | `bool Init(int w, int h, std::string_view title)` | 创建窗口（`GLFW_NO_API`）。 |
| `Shutdown` | `void Shutdown() noexcept` | |
| `PollEvents` | `void PollEvents() const` | 非阻塞。 |
| `ShouldClose` | `bool ShouldClose() const` | |
| `GetFramebufferSize` | `void GetFramebufferSize(int& w, int& h) const` | |
| `GetWindowSize` | `void GetWindowSize(int& w, int& h) const` | |
| `GetNativeHandle` | `GLFWwindow* GetNativeHandle() const noexcept` | 交给 `TgfxRenderer::Init`。 |
| `GetCursorPos` | `Point GetCursorPos() const noexcept` | |

输入回调设置器：`SetInputCallback`、`SetScrollCallback`、`SetResizeCallback`、
`SetMouseMoveCallback`。

## gflag 参考

所有标志从传给 `Application::Init` 的 `argc/argv` 解析。

| 标志 | 类型 | 默认值 | 含义 |
|------|------|--------|------|
| `--target_fps` | `int32` | `60` | 事件循环帧率上限。 |
| `--render_queue_capacity` | `uint64` | `2048` | SPSC 环形队列槽数，向上取整为 2 的幂；可用槽位 = 容量 - 1。 |
| `--verbose_logging` | `bool` | `false` | 启用详细（调试）日志。 |
| `--logtostderr` | `bool` | `false` | glog 输出到 stderr 而非文件。 |
| `--log_dir` | `string` | `"./logs"` | glog 日志目录。 |

### 示例

```powershell
.\my_app.exe --target_fps=30 --verbose_logging --logtostderr
```

```bash
./my_app --render_queue_capacity=4096 --log_dir=./logs
```

::: tip 后端选择在编译期
没有运行时后端参数。切换 GPU 后端需要换 `TGFX_USE_*` 开关重新跑 CMake 并
重编译——见 [GPU 后端](../guide/configuration.md)。
:::
