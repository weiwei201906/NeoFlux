# Widget 系统

NeoFlux 采用类 Flutter 的 Widget 开发模型。每个 UI 元素都是一个 `Widget`，可以包含子组件。

## 核心 Widget

| Widget | 说明 |
|--------|------|
| `Widget` | 抽象基类，重写 `Build()`、`OnMeasure()`、`Paint()` 及事件回调 |
| `Container` | Flexbox 容器，支持 padding、margin、背景色、圆角、flex direction |
| `Text` | 单行文本，支持字体大小、颜色、对齐方式，UTF-8 编码 |
| `Button` | 可点击按钮，支持标签、按下回调、按下状态样式 |
| `ScrollView` | 可滚动视口，支持滚轮与拖拽滚动，内容裁剪 |
| `Draggable` | 可拖拽容器，绘制时平移不影响布局 |
| `Expanded` | 设置了 flex_grow 的容器，填充父容器剩余空间 |
| `SizedBox` | 固定宽高的容器，用于固定间距 |

## 自定义 Widget

继承 `Widget` 并重写虚函数：

### 核心虚函数

| 函数 | 用途 |
|------|------|
| `Build(BuildContext&)` | 为有状态组件返回子 Widget |
| `OnMeasure(width, mode, height, mode)` | 叶子组件返回固有尺寸 |
| `Paint(RenderContext&)` | 生成渲染命令 |
| `HitTest(Point)` | 测试点是否命中此 Widget |
| `OnPointerDown(const Point&)` | 处理按下事件（返回 true 表示消费） |
| `OnPointerUp(const Point&)` | 处理释放事件 |
| `OnPointerMove(const Point&)` | 处理指针移动（悬停/拖拽） |
| `OnPointerEnter()` | 指针进入 Widget 边界 |
| `OnPointerExit()` | 指针离开 Widget 边界 |
| `OnPointerScroll(const Point&, x, y)` | 处理滚动事件 |
| `GetWidgetName()` | 返回 Widget 名称（string_view，用于调试） |

```cpp
class MyWidget : public Widget {
 public:
  std::shared_ptr<Widget> Build(BuildContext& ctx) override {
    auto container = std::make_shared<Container>();
    // 构建子组件树
    return container;
  }

  void Paint(RenderContext& ctx) override {
    // 自定义绘制
    Widget::Paint(ctx);
  }
};
```

## 状态管理

- `StatelessWidget`：无状态组件基类
- `StatefulWidget`：有状态组件基类，配合 `State<W>` 使用
- `SetState()`：标记状态变化，触发重建

## 下一步

- [Flex 布局](./layout)
- [输入与事件](./input)
