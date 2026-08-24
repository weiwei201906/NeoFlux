# 性能优化指南

NeoFlux 设计为轻量级、高性能的 UI 渲染框架。本指南涵盖优化技术和最佳实践。

## 架构级优化

### 绘制时变换 vs. 布局更改

视觉上移动的 Widget（例如 `Draggable`、`ScrollView`）应使用绘制时变换，而非更改布局属性：

```cpp
// 好：绘制时平移，不触发 Taitank 重新布局
void Draggable::Paint(RenderContext& ctx) {
  ctx.Save();
  ctx.Translate(drag_offset_.x, drag_offset_.y);
  Container::Paint(ctx);
  ctx.Restore();
}

// 不好：更改布局属性触发整棵 Taitank 子树重新布局
void Draggable::OnPointerMove(const Point& p) {
  SetX(bounds_.x + p.x);  // 触发整个子树重新布局
}
```

绘制时变换每帧 $O(1)$；布局更改 $O(n)$，$n$ 为子树大小。

### 脏帧语义

为更改使用正确的脏标记：

| 更改 | 方法 | 开销 |
|------|------|------|
| Widget 树结构 | `MarkNeedsBuild()` | Build + Layout + Paint |
| 布局属性 | `MarkFrameDirty()` | Layout + Paint |
| 仅绘制（偏移、滚动） | `MarkFrameDirty()` | 仅 Paint（布局缓存） |

:::tip
指针事件自动调用 `MarkFrameDirty()`。不要在拖拽/滚动操作的 `OnPointerMove()` 中调用 `MarkNeedsBuild()`。
:::

### 命中测试缓存

`Application` 为指针移动事件缓存上一次命中测试结果。布局变化时缓存失效。对于大型 Widget 树（>1000 个 widget），这避免了每次鼠标移动都遍历整棵树。

## 位运算优化

NeoFlux 在热路径中全程使用位运算：

### 环形队列索引回绕

```cpp
mask_ = capacity_ - 1;            // 2 的幂减一
next_head = (head + 1) & mask_;   // 1 周期 vs ~20-40 周期取模
```

### 帧率日志

```cpp
if ((frames_rendered & 63U) == 0U) {  // 每 64 帧（2^6）
  LOG(INFO) << "Rendered " << frames_rendered << " frames";
}
```

### 奇偶判断

```cpp
if ((index & 1U) == 0U) {  // 偶数索引，比 index % 2 == 0 快
  // ...
}
```

:::warning
位移（`>>`、`<<`）仅适用于整数类型。浮点数除法（`/ 2.0F`）不能使用位运算。
:::

## 内存优化

### 最小类型

使用能容纳值的最小类型：

```cpp
// 窗口尺寸不会超过 65535
std::uint16_t window_width_ = 800;
std::uint16_t window_height_ = 600;

// 值很少的枚举使用 uint8_t 底层类型
enum class WidgetState : std::uint8_t { kIdle, kHovering, kDragging };
```

### 智能指针

- `std::unique_ptr` 用于独占所有权（渲染器、平台桥接）
- `std::shared_ptr` 用于 Widget 树（与父/子组件共享所有权）
- `std::weak_ptr` 用于交叉引用（按下的 widget、悬停的 widget、命中缓存）以避免引用循环

### 字符串视图

非持有字符串参数使用 `std::string_view`：

```cpp
void SetFontDir(std::string_view dir) noexcept;  // 无拷贝
```

仅在需要所有权时（例如存储为成员）才转换为 `std::string`。

## 渲染优化

### SPSC 环形队列

渲染队列是无锁单生产者单消费者环形缓冲区：
- 主线程和渲染线程之间无互斥锁竞争
- 缓存行对齐的 head/tail 计数器避免伪共享
- `std::construct_at`/`std::destroy_at` 支持非平凡类型

### 帧批处理

一帧的所有渲染命令在 `kBeginFrame` 和 `kEndFrame` 之间批处理。渲染线程每次唤醒处理一整帧，最小化上下文切换。

### 信号量阻塞的渲染线程

渲染线程在无帧就绪时阻塞在信号量上，消耗零 CPU。仅在主线程提交帧时唤醒。

## 性能分析

### 带调试符号构建

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

### 详细日志

```bash
./my_app --verbose_logging=true
```

日志包含帧数、渲染队列状态和 Widget 构建周期。

### 帧率

检查日志中的 "Rendered N frames" —— 每 64 帧打印一次。与目标 FPS（`--target_fps`，默认 60）对比。

## 常见性能陷阱

1. **在 `OnPointerMove()` 中调用 `MarkNeedsBuild()`** —— 每次鼠标事件触发完整 Widget 重建。改用绘制时变换。
2. **大型 Widget 树无虚拟化** —— `ScrollView` 尚不支持懒加载 Widget 创建。超过 1000 项的列表考虑手动 Widget 回收。
3. **频繁更改字体** —— 每个不同字体的 `Text` widget 触发字体加载。跨 Widget 复用字体。
4. **阻塞主线程** —— 事件处理函数或协程中的长操作阻塞帧处理。使用 `co_await Yield()` 将工作分散到多帧。
