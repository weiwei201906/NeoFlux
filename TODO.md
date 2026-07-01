# TODO: neoflux 开发任务清单（TDD 驱动）

> **开发准则**：
> - **TDD（红-绿-重构）**：先写测试（红），再实现（绿），最后重构（重构），确保 `clang-tidy` 无警告。
> - **Google C++ Style**：所有代码必须符合，使用 `clang-format` 格式化和 `clang-tidy` 检查。
> - **命名空间**：全部置于 `neoflux` 下，宏以 `NEOFLUX_` 开头。
> - **增量提交**：每完成一个子任务即提交，保持 `main` 分支可构建。
> - **许可证**：GPLv3，所有源文件头必须包含版权声明。

---

## 阶段 0：项目初始化与环境配置

### 0.1 编写根 CMakeLists.txt
- **文件**：`/CMakeLists.txt`
- **内容**：
  - 设置 CMake 最低版本 3.20，项目名 `neoflux`，版本号 1.0.0。
  - 设置 C++20 标准，启用 LTO（`CMAKE_INTERPROCEDURAL_OPTIMIZATION`），Release 模式添加 `-march=native -O3`。
  - 添加 `cmake/` 到 `CMAKE_MODULE_PATH`。
  - `include(cmake/Dependencies.cmake)` 和 `find_package(Skia REQUIRED)`。
  - `add_subdirectory(framework)`、`add_subdirectory(app)`、`add_subdirectory(tests)`。
  - 导出编译数据库 `CMAKE_EXPORT_COMPILE_COMMANDS ON`。
  - 添加 GPLv3 许可证头注释（作为 CMake 注释）。

### 0.2 编写 cmake/Dependencies.cmake
- **文件**：`cmake/Dependencies.cmake`
- **内容**：
  - 使用 `FetchContent` 声明：
    - `glog`（Git 标签 v0.6.0）
    - `googletest`（标签 v1.14.0）
    - `taitank`（GitHub: Tencent/Taitank，分支 main）
  - 调用 `FetchContent_MakeAvailable` 使它们成为可用目标。

### 0.3 编写 cmake/FindSkia.cmake
- **文件**：`cmake/FindSkia.cmake`
- **内容**：
  - 在 `thirdparty/skia/include` 和 `thirdparty/skia/lib` 中查找头文件和 `libskia.a`。
  - 若未找到，输出错误提示“请运行 thirdparty/skia/download_skia.sh”。
  - 定义导入目标 `Skia::Skia`，设置 `IMPORTED_LOCATION` 和 `INTERFACE_INCLUDE_DIRECTORIES`。

### 0.4 编写 framework/CMakeLists.txt
- **文件**：`framework/CMakeLists.txt`
- **内容**：
  - 定义静态库目标 `neoflux_core`。
  - 列出所有源文件（目前为空，后续逐步添加）。
  - 设置 `PUBLIC` 包含目录 `include`，`PRIVATE` 包含 `src`。
  - 链接依赖：`glog::glog`、`taitank`、`Skia::Skia`、`Threads::Threads`。
  - 添加编译选项 `-Wall -Wextra -Wpedantic`。
  - 每个源文件头添加 GPLv3 版权声明（可通过 `configure_file` 或脚本生成）。

### 0.5 编写 app/CMakeLists.txt 和 tests/CMakeLists.txt
- **app/CMakeLists.txt**：
  - 添加可执行文件 `neoflux_app`，源文件 `src/main.cpp` 等。
  - 链接 `neoflux_core` 和 `glog::glog`。
- **tests/CMakeLists.txt**：
  - 添加可执行文件 `neoflux_test`，源文件 `src/test_main.cpp` 及后续测试文件。
  - 链接 `neoflux_core`、`gtest`、`gtest_main`、`glog::glog`。
  - 调用 `enable_testing()` 和 `add_test()`。

### 0.6 添加代码规范配置文件
- **.clang-format**：基于 Google 风格，列宽 120，缩进 2 空格。
- **.clang-tidy**：启用 `google-*`、`performance-*`、`modernize-*`、`cppcoreguidelines-*`、`bugprone-*`，排除 `cppcoreguidelines-avoid-magic-numbers`（测试中允许）。
- **scripts/format.sh**：遍历 `framework/src`、`framework/include`、`app/src`、`tests/src` 中的 `.cpp/.h`，调用 `clang-format -i`。
- **scripts/lint.sh**：使用 `clang-tidy` 对所有源文件检查（需 `compile_commands.json`）。

### 0.7 创建占位 main 和 test_main
- **app/src/main.cpp**：初始化 GLog，输出 "neoflux App started"，返回 0。文件头包含 GPLv3 版权声明。
- **tests/src/test_main.cpp**：初始化 GLog 和 GTest，运行所有测试（目前为空测试套件）。文件头包含 GPLv3 版权声明。
- **验收**：`./build/app/neoflux_app` 输出日志，`./build/tests/neoflux_test` 通过空测试。

---

## 阶段 1：并发原语（无锁队列、信号量、线程池）

### 1.1 SPSC 无锁环形队列（基于 mmap）
- **测试文件**：`tests/src/test_ringbuffer.cpp`（文件头包含 GPLv3）
  - **红**：编写以下测试用例：
    1. `TEST(SPSCRingBuffer, SinglePushPop)`：创建一个容量为 4、元素大小为 8 的队列，push 一个整数，pop 出来，验证相等。
    2. `TEST(SPSCRingBuffer, BatchPushPop)`：批量 push 3 个，批量 pop 3 个，顺序正确。
    3. `TEST(SPSCRingBuffer, FullQueue)`：填满队列后，`try_push` 返回 false。
    4. `TEST(SPSCRingBuffer, EmptyQueue)`：空队列 `try_pop` 返回 false。
    5. `TEST(SPSCRingBuffer, ConcurrentProducerConsumer)`：启动两个线程，生产者 push 10000 个，消费者 pop，验证全部消费且顺序一致（可使用原子计数器记录序号）。
- **实现文件**：`framework/src/threading/SPSCRingBuffer.h` 和 `.cpp`（文件头包含 GPLv3）
  - **绿**：
    - 类 `SPSCRingBuffer`，构造函数接受 `capacity`（2 的幂）和 `item_size`。
    - 内部使用 `mmap` 分配 `sizeof(Meta) + capacity * item_size`，`Meta` 包含 `std::atomic<uint32_t> head, tail; uint32_t mask; uint32_t item_size;`，对齐到 64 字节。
    - 实现 `try_push(const void*)`、`try_push_batch(const void*, size_t)`、`try_pop(void*)`、`try_pop_batch(void*, size_t)`。
    - 内存序：`head` 用 `memory_order_acquire` 读，`tail` 用 `memory_order_release` 写。
    - 析构函数 `munmap`。
  - **重构**：
    - 添加 `reset()` 用于测试。
    - 添加 `empty()` 和 `size()` 辅助（仅调试）。
    - 确保 `static_assert` 检查 `Meta` 大小不超过 64 字节。
- **验收**：所有测试通过，`clang-tidy` 无警告（对 `mmap` 可能需 `NOLINT`）。

### 1.2 信号量（Semaphore）
- **测试文件**：`tests/src/test_semaphore.cpp`（文件头包含 GPLv3）
  - **红**：
    1. `TEST(Semaphore, Basic)`：初始 0，release(1)，acquire 成功。
    2. `TEST(Semaphore, Multiple)`：release(3)，acquire 三次成功。
    3. `TEST(Semaphore, TryAcquireTimeout)`：初始 0，`try_acquire_for(10ms)` 返回 false，release 后返回 true。
    4. `TEST(Semaphore, ThreadWait)`：一个线程 acquire 阻塞，另一线程 10ms 后 release，阻塞线程被唤醒。
- **实现文件**：`framework/src/threading/Semaphore.h` 和 `.cpp`（文件头包含 GPLv3）
  - **绿**：使用 C++20 `std::counting_semaphore<>` 封装，提供 `release(int n=1)`、`acquire()`、`try_acquire()`、`try_acquire_for(std::chrono::milliseconds)`。
- **重构**：无额外需求。
- **验收**：测试通过，`clang-tidy` 通过。

### 1.3 线程池（用于布局线程）
- **测试文件**：`tests/src/test_threadpool.cpp`（文件头包含 GPLv3）
  - **红**：
    1. `TEST(ThreadPool, SingleTask)`：提交一个返回 int 的任务，获取 future，结果正确。
    2. `TEST(ThreadPool, MultipleTasks)`：提交 10 个任务，并行执行，全部正确。
    3. `TEST(ThreadPool, WaitAll)`：提交 5 个任务，调用 `wait_all()` 阻塞直到完成。
- **实现文件**：`framework/src/threading/ThreadPool.h` 和 `.cpp`（文件头包含 GPLv3）
  - **绿**：
    - 构造时创建 `num_threads` 个工作线程。
    - 使用 `std::queue<std::function<void()>>` + `std::mutex` + `std::condition_variable`（初期简单实现，后续可替换为无锁队列）。
    - `enqueue(F&& f, Args&&...)` 返回 `std::future<ResultType>`，内部使用 `std::packaged_task`。
    - `wait_all()` 使用原子计数或条件变量。
    - 析构时停止所有线程。
  - **重构**：若性能瓶颈，可替换为 SPSC 无锁队列。
- **验收**：测试通过，`clang-tidy` 通过。

---

## 阶段 2：工具类（SIMD 加速、内存池）

### 2.1 SIMD 工具函数
- **测试文件**：`tests/src/test_simd.cpp`（文件头包含 GPLv3）
  - **红**：
    1. `TEST(SIMDUtils, TransformPositions)`：生成 100 个随机坐标对，调用 `simd::transformPositions` 加上偏移，与标量计算对比。
    2. `TEST(SIMDUtils, MemcpySimd)`：拷贝 1024 字节随机数据，与 `std::memcpy` 结果一致。
    3. `TEST(SIMDUtils, AddFloat)`：随机两个数组，SIMD 加法与标量加法对比。
- **实现文件**：`framework/src/utils/SIMDUtils.h` 和 `.cpp`（文件头包含 GPLv3）
  - **绿**：
    - 命名空间 `neoflux::simd`。
    - 使用 `#ifdef __AVX2__` 等实现 AVX2 版本（`<immintrin.h>`），NEON 版本（`<arm_neon.h>`），否则提供标量 fallback。
    - 函数原型：
      - `void transformPositions(float* positions, size_t count, float dx, float dy);`
      - `void memcpy_simd(void* dst, const void* src, size_t bytes);`
      - `void add_float(const float* a, const float* b, float* out, size_t n);`
    - 注意对齐：使用 `_mm256_loadu_ps` 允许未对齐。
  - **重构**：将 SIMD 选择封装为模板特化或策略类。
- **验收**：测试通过，`clang-tidy` 对 intrinsic 可能警告，可局部禁用。

### 2.2 mmap 内存池
- **测试文件**：`tests/src/test_memorypool.cpp`（文件头包含 GPLv3）
  - **红**：
    1. `TEST(MemoryPool, Allocate)`：分配 10 个 block（每个 64 字节），地址互不重叠。
    2. `TEST(MemoryPool, Reset)`：分配后 reset，再分配地址从头部开始。
    3. `TEST(MemoryPool, Exhaustion)`：分配超出总容量，返回 nullptr。
- **实现文件**：`framework/src/utils/MemoryPool.h` 和 `.cpp`（文件头包含 GPLv3）
  - **绿**：
    - `MemoryPool(size_t block_size, size_t total_bytes)`：使用 `mmap` 分配总大小，维护 `offset_` 和 `total_bytes_`。
    - `void* allocate()`：若 `offset_ + block_size <= total_bytes_`，返回 `(char*)base_ + offset_`，`offset_ += block_size`，否则返回 `nullptr`。
    - `void reset()`：`offset_ = 0`。
    - 析构 `munmap`。
- **重构**：可添加对齐参数确保返回地址对齐到 64 字节。
- **验收**：测试通过，`clang-tidy` 通过。

---

## 阶段 3：中间层（Bridge + Flattener）

### 3.1 定义 FlatNode 结构
- **文件**：`framework/src/middleware/FlatNode.h`（内部使用，不公开，文件头包含 GPLv3）
- **内容**：
  ```cpp
  struct alignas(64) FlatNode {
      uint32_t node_id;
      float x, y, width, height;
      uint32_t flags;      // 可见性、裁剪等
      float opacity;
      uint32_t reserved[2];
  };
  static_assert(sizeof(FlatNode) <= 64);
  ```

### 3.2 FrameFlattener（拍平树 + 绝对坐标）
- **测试文件**：`tests/src/test_flattener.cpp`（文件头包含 GPLv3）
  - **红**：
    1. 构建简单树：`Container`（padding=10）包含 `Text`，期望 Text 的绝对坐标为 (10, 10, ...)（依赖 Taitank 计算结果）。
    2. 嵌套 `Row` 和 `Column`，验证坐标累加正确。
- **实现文件**：`framework/src/middleware/FrameFlattener.h` 和 `.cpp`（文件头包含 GPLv3）
  - **绿**：
    - `class FrameFlattener`，提供 `flatten(Widget* root, MemoryPool& pool)` 返回 `std::span<const FlatNode>`。
    - 内部递归遍历 Widget 树，从每个 Widget 的 `LayoutNode` 获取相对坐标，累加父绝对坐标。
    - 使用 `MemoryPool` 分配 `FlatNode` 数组。
  - **重构**：递归改为迭代（可选），避免栈溢出。
- **验收**：测试通过，`clang-tidy` 通过。

### 3.3 LayoutRenderBridge（双缓冲 + 环形队列 + 信号量）
- **测试文件**：`tests/src/test_bridge.cpp`（文件头包含 GPLv3）
  - **红**：
    1. `TEST(Bridge, PushConsume)`：push 一帧（包含几个 FlatNode），consume 得到相同数据。
    2. `TEST(Bridge, MultipleFrames)`：连续 push 3 帧，consume 应得到最新帧（旧帧可丢弃）。
    3. `TEST(Bridge, Overflow)`：push 超过容量，返回 false。
- **实现文件**：`framework/src/middleware/LayoutRenderBridge.h` 和 `.cpp`（文件头包含 GPLv3）
  - **绿**：
    - `LayoutRenderBridge(size_t ring_capacity, size_t max_nodes_per_frame)` 创建两个 `SPSCRingBuffer`（写缓冲和读缓冲）。
    - `bool pushFrame(const std::vector<FlatNode>& nodes, uint64_t frame_id)`：将 nodes 拷贝到写缓冲，成功后交换缓冲（原子操作），更新 `latest_frame_id_`，`release` 信号量。
    - `std::span<const FlatNode> consumeLatestFrame()`：获取 `active_read_ptr_` 指向的缓冲，返回其数据。
    - 交换时使用原子指针，确保消费者不会读取到正在写入的缓冲。
  - **重构**：添加帧计数统计。
- **验收**：测试通过，`clang-tidy` 通过。

---

## 阶段 4：引擎封装（Taitank + Skia + 平台窗口）

### 4.1 Taitank 布局引擎封装
- **测试文件**：`tests/src/test_layoutengine.cpp`（文件头包含 GPLv3）
  - **红**：
    1. 创建 `LayoutNode`，设置固定宽高，`doLayout` 后尺寸正确。
    2. 创建父节点和子节点，设置 flexGrow，验证比例分配。
- **实现文件**：`framework/src/engine/LayoutEngine.h` 和 `.cpp`（文件头包含 GPLv3）
  - **绿**：
    - `class LayoutNode`：RAII 封装 `TaitankNodeRef`，提供 `setStyle`（设置 flex 属性）、`addChild`。
    - `class LayoutEngine`：`doLayout(LayoutNode* root, float width, float height)`，以及 `getX/Y/Width/Height` 等获取结果。
- **重构**：`Style` 定义为结构体，便于传递。
- **验收**：测试通过，`clang-tidy` 通过。

### 4.2 Skia 渲染引擎封装
- **测试文件**：`tests/src/test_renderengine.cpp`（离屏渲染，文件头包含 GPLv3）
  - **红**：
    1. 创建 `RenderEngine(100,100)`，绘制红色矩形，snapshot 后检查像素（至少一个红色）。
    2. 绘制文本，检查像素非空。
- **实现文件**：`framework/src/engine/RenderEngine.h` 和 `.cpp`（文件头包含 GPLv3）
  - **绿**：
    - `RenderEngine(int width, int height)` 创建 `SkSurface`（Raster）。
    - `beginFrame()` / `endFrame()` 获取 `SkCanvas`。
    - `drawRect`、`drawText` 等直接调用 Skia API。
    - `snapshot()` 返回 `sk_sp<SkImage>`。
- **重构**：抽象 `RenderTarget` 接口，便于不同后端。
- **验收**：测试通过，`clang-tidy` 通过。

### 4.3 平台窗口抽象（PlatformWindow）
- **测试**：暂不编写单元测试（依赖平台），在示例中验证。
- **实现文件**：`framework/src/platform/PlatformWindow.h` 和 `.cpp`（文件头包含 GPLv3）
  - **绿**：使用 GLFW 或 SDL（可先使用 GLFW，通过 FetchContent 添加），提供 `createWindow`、`pollEvents`、`swapBuffers`、`setVSyncCallback`。若不用 GLFW，可提供 X11/Win32 实现，但初期建议 GLFW。
  - 注意：GLFW 需额外依赖，在 `Dependencies.cmake` 中添加 `FetchContent_Declare(glfw ...)` 或要求系统安装。
- **验收**：示例应用能创建窗口并显示。

---

## 阶段 5：核心抽象（Widget、State、BuildContext、InheritedWidget）

### 5.1 Widget 基类
- **测试文件**：`tests/src/test_widget.cpp`（文件头包含 GPLv3）
  - **红**：
    1. `TEST(Widget, DirtyFlag)`：创建 Widget，`markDirty()` 后 dirty 为 true。
    2. `TEST(Widget, Lifecycle)`：模拟挂载/卸载，回调执行（可设置 spy 标志）。
    3. `TEST(Widget, Build)`：子类重写 `build` 返回子 Widget 列表，验证数量。
- **实现文件**：`framework/include/neoflux/core/Widget.h`（文件头包含 GPLv3）
  - **绿**：
    - 抽象类 `Widget`，纯虚 `build(BuildContext&)` 默认返回空。
    - 提供 `markDirty()`、`setParent()`、`getParent()`、`getID()`。
    - 生命周期虚函数 `onMount/onUnmount/onUpdate`。
    - 持有 `LayoutNode*` 指针（由 LayoutEngine 管理）。
- **重构**：使用 `std::unique_ptr` 管理子 Widget。
- **验收**：测试通过，`clang-tidy` 通过。

### 5.2 State 模板类
- **测试文件**：`tests/src/test_state.cpp`（文件头包含 GPLv3）
  - **红**：`TEST(State, SetState)`：设置初始值，调用 `setState` 更新，回调被触发。
- **实现文件**：`framework/include/neoflux/core/State.h`（文件头包含 GPLv3）
  - **绿**：模板类 `State<T>`，持有 `T data_` 和 `std::function<void()> callback_`，提供 `setState(updater)` 和 `getData()`。
- **验收**：测试通过。

### 5.3 BuildContext 和 InheritedWidget
- **测试文件**：`tests/src/test_context.cpp` 和 `test_inherited.cpp`（文件头包含 GPLv3）
  - **红**：
    1. 定义 `ThemeData`，创建 `ThemeWidget`（继承 `InheritedWidget<ThemeData>`），在其下创建子 Widget，子 Widget 通过 `context.dependOnInheritedWidget<ThemeWidget>()` 获取数据，验证成功。
    2. 若无匹配，返回 nullptr。
- **实现文件**：
  - `framework/include/neoflux/core/BuildContext.h`（文件头包含 GPLv3）：类 `BuildContext`，持有 `Widget*`，提供模板方法 `dependOnInheritedWidget<T>()`，向上遍历父链，`dynamic_cast` 检查。
  - `framework/include/neoflux/widgets/InheritedWidget.h`（文件头包含 GPLv3）：模板类 `InheritedWidget<DataType>`，继承 `Widget`，存储 `DataType`，重写 `build` 返回子列表，提供 `updateShouldNotify` 虚函数。
- **验收**：测试通过。

---

## 阶段 6：内置组件（Text、Button、Container、Row/Column）

### 6.1 TextWidget
- **文件**：
  - `framework/include/neoflux/widgets/TextWidget.h`（文件头包含 GPLv3）
  - `framework/src/widgets/TextWidget.cpp`（文件头包含 GPLv3）
  - `tests/src/test_textwidget.cpp`（文件头包含 GPLv3）
- **功能**：显示文本，支持字体大小、颜色、对齐。
- **测试**：创建 TextWidget，设置文本，渲染到离屏，检查像素非空。
- **实现**：在 `draw` 中调用 Skia 的 `SkFont` 和 `SkPaint` 绘制。

### 6.2 ButtonWidget
- **文件**：
  - `framework/include/neoflux/widgets/ButtonWidget.h`（文件头包含 GPLv3）
  - `framework/src/widgets/ButtonWidget.cpp`（文件头包含 GPLv3）
  - `tests/src/test_button.cpp`（文件头包含 GPLv3）
- **功能**：可点击按钮，支持按下、悬停状态，回调。
- **测试**：模拟鼠标点击，验证回调执行。
- **实现**：内部组合 `TextWidget` 和 `Container`，或在 `draw` 中直接绘制背景和文本，并处理鼠标事件。

### 6.3 ContainerWidget
- **文件**：
  - `framework/include/neoflux/widgets/ContainerWidget.h`（文件头包含 GPLv3）
  - `framework/src/widgets/ContainerWidget.cpp`（文件头包含 GPLv3）
  - `tests/src/test_container.cpp`（文件头包含 GPLv3）
- **功能**：装饰容器，提供 `padding`、`margin`、`background`、`border`、`clip` 等。
- **实现**：在 `build` 中返回子 Widget，并在 Taitank 节点上设置相应的样式（margin/padding）。

### 6.4 RowWidget 和 ColumnWidget
- **文件**：
  - `framework/include/neoflux/widgets/RowWidget.h`（文件头包含 GPLv3）
  - `framework/include/neoflux/widgets/ColumnWidget.h`（文件头包含 GPLv3）
  - `framework/src/widgets/RowWidget.cpp`（文件头包含 GPLv3）
  - `framework/src/widgets/ColumnWidget.cpp`（文件头包含 GPLv3）
  - `tests/src/test_rowcolumn.cpp`（文件头包含 GPLv3）
- **功能**：水平/垂直排列子 Widget，支持 flex 属性。
- **实现**：继承 `Widget`，在 `build` 中返回子列表，在 `onMount`/`onUpdate` 中为每个子 Widget 设置 Taitank 的 flex 属性（`flexGrow`、`flexShrink`、`alignSelf` 等）。

---

## 阶段 7：应用层（示例、事件循环、VSync）

### 7.1 主循环（Application 类）
- **文件**：`app/src/main.cpp`（文件头包含 GPLv3）
- **功能**：
  - 创建 `PlatformWindow`，设置 VSync 回调。
  - 主循环：`pollEvents()`，若需要渲染则 `renderFrame()`。
  - `renderFrame`：从 `LayoutRenderBridge` 消费最新帧，遍历 `FlatNode` 调用 `RenderEngine` 绘制，最后 `swapBuffers`。
- **测试**：手动运行 `neoflux_app`，验证窗口显示。

### 7.2 事件分发（鼠标/键盘 → Widget 交互）
- **实现**：在 `PlatformWindow` 中捕获事件，通过 `hitTest` 找到目标 Widget，调用其 `onMouseDown/Up/Move` 等方法（需要在 `Widget` 中添加这些虚函数）。
- **测试**：可模拟鼠标事件，验证 Button 回调。

### 7.3 示例应用：CounterWidget
- **文件**：
  - `app/src/CounterWidget.h`（文件头包含 GPLv3）
  - `app/src/CounterWidget.cpp`（文件头包含 GPLv3）
  - `app/src/MyAppWidget.h`（文件头包含 GPLv3）
  - `app/src/MyAppWidget.cpp`（文件头包含 GPLv3）
- **实现**：构建根 Widget（MyApp），包含 CounterWidget，展示点击增加计数。
- **验收**：运行 `neoflux_app`，交互正常。

---

## 阶段 8：集成测试和性能基准

### 8.1 端到端集成测试
- **文件**：`tests/src/integration_test.cpp`（文件头包含 GPLv3）
- **内容**：创建 Widget 树，模拟若干帧，离屏渲染，对比预期像素（可用简单的颜色块验证）。
- **实现**：使用 `RenderEngine` 离屏，手动驱动 `Application` 循环，验证输出。

### 8.2 性能基准测试
- **文件**：
  - `tests/benchmarks/bench_layout.cpp`（文件头包含 GPLv3）
  - `tests/benchmarks/bench_ringbuffer.cpp`（文件头包含 GPLv3）
  - `tests/benchmarks/bench_render.cpp`（文件头包含 GPLv3）
- **内容**：
  - 布局性能：1000 个 Widget 的布局耗时。
  - 队列吞吐量：每秒 push/pop 操作数。
  - 渲染帧率：绘制 500 个矩形的时间。
- **记录**：结果记录在 `docs/benchmarks.md`。

---

## 阶段 9：文档完善与发布准备

### 9.1 API 文档
- 使用 Doxygen 为 `include/` 中的头文件生成注释。
- 编写 `docs/api.md`、`docs/design.md`、`docs/performance.md`。

### 9.2 添加 LICENSE 文件
- **文件**：`/LICENSE`
- **内容**：完整的 GPLv3 许可证文本（从 https://www.gnu.org/licenses/gpl-3.0.txt 获取）。

### 9.3 代码全面清理
- 运行 `./scripts/format.sh` 格式化所有代码。
- 运行 `./scripts/lint.sh` 修复所有 clang-tidy 警告。
- 确保所有测试通过。

### 9.4 安装与打包
- 在根 `CMakeLists.txt` 中添加 `install(TARGETS neoflux_core ...)`。
- 提供 `Findneoflux.cmake` 模块供外部项目使用。
- 打包 Release（静态库 + 头文件）。
- 创建 GitHub Release v1.0.0，附带 Changelog。

---

> **所有子任务完成后，框架即达到可用状态。** 每个阶段均可独立验证，建议按顺序推进，每完成一个阶段就提交一次代码。
