# 配置

NeoFlux 使用 gflags 进行运行时配置，所有参数均为可选。

## 命令行参数

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `--target_fps` | int | `60` | 应用事件循环与渲染的目标帧率 |
| `--render_queue_capacity` | int | `2048` | SPSC 无锁环形队列容量，自动向上取整为 2 的幂 |
| `--verbose_logging` | bool | `false` | 启用详细 VLOG(1) 输出并将日志镜像到 stderr |
| `--logtostderr` | bool | `false` | 将日志输出到 stderr 而非日志文件 |
| `--log_dir` | string | `./logs` | 日志文件存放目录，不存在时自动创建 |
| `--render_backend` | string | `vulkan` | 渲染后端：`vulkan`、`gl`、`cpu` |

## 日志配置

默认日志输出到 `./logs/` 文件，Windows 下不显示控制台窗口（`CMAKE_WIN32_EXECUTABLE`）。

调试时使用：

```bash
./bin/hello_neoflux --logtostderr --verbose_logging
```

## 渲染后端

`--render_backend` 支持以下选项：

- `vulkan`（默认）：Vulkan 渲染，当前回退到 OpenGL
- `gl`：OpenGL 渲染
- `cpu`：软件光栅化，当前回退到 OpenGL

未实现的后端会输出警告并回退到 OpenGL。

## CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `NEOFLUX_BUILD_TESTS` | `OFF` | 编译单元测试 |

启用测试：

```bash
cmake -S . -B build -DNEOFLUX_BUILD_TESTS=ON
```

## 下一步

- [跨平台](./cross-platform)
- [测试](./testing)
