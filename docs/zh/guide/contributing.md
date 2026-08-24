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

## 提交前必读

:::danger
在本地验证所有内容之前，请勿提交 PR。
:::

### 代码质量

- **干净且最优**：审查代码中不必要的分配、冗余拷贝和遗漏的优化机会。非持有参数优先用 `std::string_view` 而非 `std::string`，结构体用 designated initializers，适当场景使用 C++20 ranges/views。
- **位运算优化**：将 2 的幂的整数除法/取模替换为位移和掩码。务必注释意图：
  - `x / 2` → `x >> 1`（整数减半）
  - `x % 2 == 0` → `(x & 1U) == 0U`（奇偶判断）
  - `x % 64` → `x & 63U`（2 的幂边界回绕）
  - 环形队列使用 `mask_ = capacity - 1` 配合 `index & mask_` 替代 `index % capacity`（1 周期 AND vs ~20-40 周期除法）。
  浮点数除法（`/ 2.0F`）**不能**使用位移。
- **禁止裸持有指针**：使用 `std::unique_ptr` / `std::shared_ptr` / `std::weak_ptr`。新代码中绝不使用 `new`/`delete`。
- **RAII**：所有资源（文件句柄、GPU 上下文、内存）必须由 RAII 包装管理。
- **纯 ASCII**：所有源代码、注释、日志消息和字符串字面量必须为 ASCII。`.h`/`.cpp` 中不允许出现非 ASCII 字符（包括中文）。
- **头文件仅含声明**：所有实现放在 `.cpp`。模板类使用 `.inc` + 显式实例化。

### 本地复现（必须）

开启 PR 之前，你**必须**在本地验证：

1. **干净构建**：删除构建目录，从头配置：
   ```bash
   rm -rf build
   cmake -S . -B build -DNEOFLUX_BUILD_TESTS=ON -DNEOFLUX_BUILD_EXAMPLES=ON
   cmake --build build
   ```
2. **零警告**：构建必须通过 `-Werror`（默认启用）。
3. **clang-tidy**：运行 clang-tidy 并修复所有警告：
   ```bash
   clang-tidy -p build src/**/*.cpp
   ```
4. **测试通过**：所有单元测试必须通过：
   ```bash
   cd build && ctest --output-on-failure
   ```
5. **示例运行**：至少运行 `hello_neoflux` 和与你改动最相关的示例，确认运行时正常工作，而不仅仅是编译通过。

未通过以上任何一项检查的 PR 将在 review 前被要求修改。

## Pull Request 工作流程

1. Fork 仓库并创建功能分支
2. 按上述规范进行修改
3. 运行 clang-tidy 并修复所有警告
4. 使用 `-Werror` 构建并确保零警告
5. 运行测试套件
6. 本地验证示例运行
7. 提交 PR，清晰描述变更内容
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
