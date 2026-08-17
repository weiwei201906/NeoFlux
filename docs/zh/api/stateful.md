# StatefulWidget

有状态 Widget 基类，配合 `State<W>` 使用。

## 概述

`StatefulWidget` 是持有可变状态的 Widget 基类。状态由独立的 `State<W>` 对象管理，状态变化时触发 Widget 重建。

## 基本用法

```cpp
class Counter : public StatefulWidget {
 public:
  std::unique_ptr<State<Counter>> CreateState() override;
};

class CounterState : public State<Counter> {
 public:
  std::shared_ptr<Widget> Build(BuildContext& ctx) override {
    auto button = std::make_shared<Button>(
        "Count: " + std::to_string(count_));
    button->SetOnPressed([this]() {
      SetState([this]() { ++count_; });
    });
    return button;
  }

 private:
  int count_ = 0;
};
```

## State 方法

| 方法 | 说明 |
|------|------|
| `Build(ctx) -> std::shared_ptr<Widget>` | 构建 Widget 树 |
| `SetState(mutator)` | 更新状态并标记重建 |
| `InitState()` | 状态初始化回调 |
| `DidChangeDependencies()` | 依赖变化回调 |
| `Dispose()` | 状态销毁回调 |

## 另见

- [Widget](./widget)
- [计数器示例](../examples/counter)
