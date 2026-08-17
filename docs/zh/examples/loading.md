# 加载动画

演示 Widget 状态机与 C++20 协程的集成。"Start Loading" 按钮将组件切换到加载状态；协程在约 2 秒内将进度条从 0% 动画到 100%，每帧 yield 一次。完成后组件切换到成功状态。

## 运行

```bash
./bin/loading_demo
```

## 核心概念

### 状态机

根组件使用 `WidgetState` 跟踪三种状态：
- `kIdle` — 初始状态，显示 "Start Loading" 按钮
- `kLoading` — 进度条动画中，显示百分比
- `kSuccess` — 加载完成，显示 "Done!" 消息

### 协程动画

加载动画由协程驱动，每帧 yield 一次：

```cpp
Task<void> LoadingCoroutine() {
  for (int i = 0; i <= 100; ++i) {
    progress_ = i / 100.0F;
    MarkNeedsBuild();
    co_await Yield();  // 下一帧恢复
  }
  SetState(WidgetState::kSuccess);
}
```

每次迭代更新进度值，标记组件 dirty，然后 yield 到下一帧。事件循环每帧恢复协程一次。

### 进度条

进度条使用 `Container` 作为轨道，子 `Container` 作为填充。轨道使用 row flex 方向，填充在内部左对齐。填充宽度与 `progress_` 成比例。

## 另见

- [协程指南](../guide/coroutines)
- [Task API](../api/task)
