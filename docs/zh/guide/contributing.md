# 贡献指南

感谢你对 NeoFlux 的贡献兴趣！本文档概述了所有 Pull Request 需遵守的编码规范和工作流程。

## 前置要求

提交 PR 前请确保：

- 支持 C++20 的编译器（GCC 11+、Clang 14+、MSVC 2022）
- CMake 3.20+
- clang-tidy（用于静态分析）
- Git

## 编码规范

### C++ 标准

NeoFlux 目标为 **C++20**。在适当场景使用现代 C++20 特性：

- `std::string_view` 用于非持有字符串参数
- Designated initializers 初始化结构体
- Ranges 和 views（`std::views::filter`、`std::views::transform`）
- Concepts 和约束
- `std::span` 用于数组视图
- 协程（`Task<void>`）用于异步工作

参考：[cppreference 上的 C++20](https://en.cppreference.com/w/cpp/20)

### Google C++ 编码规范

NeoFlux 遵循 [Google C++ 编码规范](https://google.github.io/styleguide/cppguide.html)。
要点：

- 命名：类/结构体用 `PascalCase`，函数/变量用 `snake_case`，编译期常量用 `kConstantName`，成员变量用 `trailing_underscore_`
- 头文件包含：项目头文件优先，然后第三方，最后系统
- 注释：仅限英文，公共 API 用 Doxygen 风格
- 头文件中禁止 `using namespace`
- 行宽尽量控制在 80 字符以内

### RAII 与智能指针

:::danger
新代码中禁止使用裸 `new`/`delete` 或裸持有指针。
:::

- 独占所有权用 `std::unique_ptr`
- 共享所有权用 `std::shared_ptr`（Widget 树使用此方式）
- 非持有观察者用 `std::weak_ptr`（尤其在协程中）
- 非持有字符串参数用 `std::string_view`
- 所有资源用 RAII 包装（文件句柄、GPU 上下文等）

参考：[cppreference 上的 RAII](https://en.cppreference.com/w/cpp/language/raii)
参考：[cppreference 上的智能指针](https://en.cppreference.com/w/cpp/memory)

### 头文件仅含声明

:::warning
头文件（`.h`）只能包含声明。所有实现放在 `.cpp` 文件中。模板类使用 `.inc` 文件并在 `.cpp` 中显式实例化。
:::

这能保持编译速度并隐藏实现细节。

### 纯 ASCII

所有源代码、注释、日志消息和字符串字面量必须为纯 ASCII。`.h`/`.cpp` 文件中不允许出现非 ASCII 字符（包括中文）。

## 静态分析

### clang-tidy

所有 PR 必须通过 clang-tidy 零警告。运行：

```bash
clang-tidy -p build src/**/*.cpp include/neoflux/**/*.h
```

项目自带 `.clang-tidy` 配置。强制执行的常见检查：

- `modernize-*` — 现代 C++ 惯用法
- `performance-*` — 性能优化
- `readability-*` — 代码可读性
- `bugprone-*` — 常见 bug 模式
- `cppcoreguidelines-*` — C++ 核心准则

:::tip
如果 clang-tidy 的建议会损害可读性或性能，请在 PR 中说明原因，并添加针对性的 `NOLINT` 注释。
:::

### 编译器警告

构建使用 `-Wall -Wextra -Wpedantic -Werror`。所有 PR 必须零警告编译通过。

## 测试

- 在 `tests/` 下为新功能添加单元测试
- 提交前运行完整测试套件：

```bash
cmake -S . -B build -DNEOFLUX_BUILD_TESTS=ON
cmake --build build
cd build && ctest --output-on-failure
```

## Pull Request 工作流程

1. Fork 仓库并创建功能分支
2. 按上述规范进行修改
3. 运行 clang-tidy 并修复所有警告
4. 使用 `-Werror` 构建并确保零警告
5. 运行测试套件
6. 提交 PR，清晰描述变更内容

## 提交信息

使用约定式提交格式：

- `feat:` — 新功能
- `fix:` — Bug 修复
- `docs:` — 仅文档
- `refactor:` — 既不修复 bug 也不添加功能的代码变更
- `perf:` — 性能优化
- `test:` — 添加或更新测试
- `chore:` — 构建流程、工具或维护

## 寻求帮助

如有贡献相关问题，请在 GitHub 上开 issue。
