# 渲染器 / 渲染后端

渲染层运行在独立线程上，通过 SPSC 环形队列消费应用线程推入的渲染命令。桌面端为
GLFW/WGL 桥接，移动端为 tgfx。具体图形后端由 `--render_backend` 标志在运行时选择。

## RenderLayer

```cpp
class RenderLayer;  // 通过 Application::GetRenderLayer() 获取
```

通常不要直接构造。`Application` 持有它并通过以下方式暴露：

```cpp
RenderLayer& Application::GetRenderLayer() noexcept;
```

它拥有 GL/Vulkan 上下文、命令接收端，并负责每帧呈现。

## GlfwBridge（桌面窗口与输入）

```cpp
class GlfwBridge : public NonCopyable;
```

头文件：`<neoflux/renderers/glfw_bridge.h>`。封装 GLFW 窗口与 OpenGL 上下文，
并把系统输入翻译成 NeoFlux 回调。

| 方法 | 签名 | 说明 |
|------|------|------|
| `Init` | `bool Init(int w, int h, std::string_view title)` | 创建窗口。 |
| `Shutdown` | `void Shutdown() noexcept` | |
| `PollEvents` | `void PollEvents() const` | 非阻塞。 |
| `SwapBuffers` | `void SwapBuffers()` | 呈现。 |
| `ShouldClose` | `bool ShouldClose() const` | |
| `GetFramebufferSize` | `void GetFramebufferSize(int& w, int& h) const` | |
| `GetWindowSize` | `void GetWindowSize(int& w, int& h) const` | |
| `GetNativeHandle` | `GLFWwindow* GetNativeHandle() const noexcept` | |
| `GetCursorPos` | `Point GetCursorPos() const noexcept` | |
| `GetGlContext` | `void* GetGlContext() const noexcept` | 供 tgfx 使用。 |
| `MakeContextCurrent` | `void MakeContextCurrent()` | |
| `ReleaseContext` | `static void ReleaseContext()` | 解绑。 |

输入回调设置器：`SetInputCallback`、`SetScrollCallback`、`SetResizeCallback`、
`SetMouseMoveCallback`。

## 完整 gflag 参考

所有标志从传给 `Application::Init` 的 `argc/argv` 解析。

| 标志 | 类型 | 默认值 | 含义 |
|------|------|--------|------|
| `--render_backend` | `string` | `"vulkan"` | 图形后端：`vulkan`、`gl` 或 `cpu`。不可用时 `vulkan` 与 `cpu` 回退到 `gl` 并给出警告。 |
| `--target_fps` | `int32` | `60` | 事件循环帧率上限。 |
| `--render_queue_capacity` | `uint64` | `2048` | SPSC 环形队列槽数，向上取整为 2 的幂；可用槽位 = 容量 - 1。 |
| `--verbose_logging` | `bool` | `false` | 启用详细（调试）日志。 |
| `--logtostderr` | `bool` | `false` | glog 输出到 stderr 而非文件。 |
| `--log_dir` | `string` | `"./logs"` | glog 日志目录。 |

### 示例

```powershell
.\my_app.exe --render_backend=gl --target_fps=30 --verbose_logging --logtostderr
```

```bash
./my_app --render_backend=vulkan --render_queue_capacity=4096 --log_dir=./logs
```

::: warning 后端回退
若所选 `--render_backend` 无法创建（无 Vulkan 设备等），NeoFlux 会记录警告并回退到
OpenGL，而不是直接退出。
:::
