# 计数器

极简计数器应用，演示 `StatefulWidget` 和 `Button` 回调。

## 运行

```bash
./bin/counter
```

## 功能

- 点击按钮增加计数
- 文本实时更新计数
- flex row/column 布局

## 核心代码

```cpp
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

## 另见

- [StatefulWidget API](../api/stateful)
- [Button API](../api/button)
