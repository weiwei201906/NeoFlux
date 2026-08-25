# 测试

NeoFlux 使用 [GoogleTest](https://github.com/google/googletest) 进行单元测试和压力测试。

## 构建测试

测试默认不构建。通过 `NEOFLUX_BUILD_TESTS` CMake 选项启用：

```bash
mkdir build && cd build
cmake .. -DNEOFLUX_BUILD_TESTS=ON
cmake --build . -j
```

:::tip
测试构建需要独立的构建目录。在启用测试的目录中运行 `ctest`。
:::

## 运行测试

```bash
cd build
ctest --output-on-failure
```

或运行单个测试二进制：

```bash
./bin/ring_queue_test
./bin/ring_queue_stress_test
./bin/widget_test
./bin/route_registry_test
./bin/render_command_test
```

## 测试套件

### RingQueue 单元测试 (`ring_queue_test`)

8 个测试覆盖：

- 初始空状态
- 推入和弹出单个元素
- 从空队列弹出返回 false
- 填满到容量
- FIFO 顺序
- 回绕行为
- move-only 类型支持
- 单生产者/单消费者并发

### RingQueue 压力测试 (`ring_queue_stress_test`)

6 个压力测试，旨在捕获单元测试可能遗漏的竞态条件、内存损坏和整数溢出 bug：

| 测试 | 描述 | 规模 |
|------|------|------|
| `FifoOrderingMultiThread` | 生产者推入 0..N-1，消费者验证严格顺序 | 100 万项 |
| `NoLossNoDuplicate` | 校验和验证：推入之和 == 弹出之和 | 100 万项 |
| `MinimumCapacity` | 容量=2 恰好存 1 个元素 | 边界 |
| `FullEmptyOscillation` | 容量=4，快速满/空转换 | 10 万次操作 |
| `MoveOnlyTypeStress` | move-only 项，结束时 live_count==0（无泄漏/双重释放） | 5 万项 |
| `LargeCapacity` | 100 万槽分配 + 1 万次填充/排空 | 100 万槽 |

:::tip
压力测试使用 `std::atomic` 计数器和校验和来检测微妙的并发 bug。使用 `-DNEOFLUX_BUILD_TESTS=ON` 和 Debug 构建运行以获得最大的断言覆盖率。
:::

### Widget 测试

测试 widget 布局和属性：

- Container 固定尺寸布局
- Container 包裹子组件内容
- Widget 层级（父/子）
- 构建脏标记
- Button 按下状态

### RouteRegistry 测试

测试路由注册和查找：

- 注册和检索路由
- 未知路由处理
- 路由替换

### RenderCommand 测试

测试渲染命令创建：

- DrawRect 命令
- DrawText 命令
- Begin/EndFrame 命令
- 变换和裁剪命令

## 编写测试

在 `tests/` 中创建新测试文件：

```cpp
#include <gtest/gtest.h>
#include <neoflux/widget/container.h>

using namespace neoflux;

TEST(MyWidgetTest, BasicFunctionality) {
  Container container;
  container.SetWidth(100.0F).SetHeight(50.0F);
  EXPECT_FLOAT_EQ(container.GetWidth(), 100.0F);
  EXPECT_FLOAT_EQ(container.GetHeight(), 50.0F);
}
```

将测试添加到 `tests/CMakeLists.txt`：

```cmake
add_executable(my_test my_test.cpp)
target_link_libraries(my_test PRIVATE neoflux gtest_main)
add_test(NAME MyTest COMMAND my_test)
```

## CI/CD

持续集成运行：

```bash
cmake -B build -DNEOFLUX_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
cd build && ctest --output-on-failure
```

## clang-tidy

除单元测试外，NeoFlux 使用 clang-tidy 进行静态分析：

```bash
clang-tidy -p build src/**/*.cpp
```

`.clang-tidy` 配置强制执行 Google C++ 风格、现代 C++ 实践和 bug-prone 模式检测。

:::warning
提交 PR 前请确保 clang-tidy 零警告。CI 会在 `-Werror` 模式下运行 clang-tidy。
:::
