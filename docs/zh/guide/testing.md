# 测试

NeoFlux 使用 Google Test 进行单元测试。

## 启用测试

测试默认禁用，通过 CMake 选项启用：

```bash
cmake -S . -B build -DNEOFLUX_BUILD_TESTS=ON
cmake --build build
cd build && ctest --output-on-failure
```

## 测试覆盖

- **RingQueue**：SPSC 无锁环形队列的基本操作、并发、容量边界
- **Widget**：Widget 树构建、子组件管理、布局
- **RouteRegistry**：路由注册、构建、查询
- **RenderCommand**：渲染命令工厂方法

## 运行单个测试

```bash
cd build
./tests/ring_queue_test
./tests/widget_test
```

## 下一步

- [配置](./configuration)
- [快速开始](./quick-start)
