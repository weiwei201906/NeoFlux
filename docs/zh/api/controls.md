# 布局与控件组件

本页覆盖较小的、专用组件。每个返回具体 widget 引用的 setter 标注为
**（可链式，返回 `X&`）**。

---

## Divider 分割线

```cpp
class Divider : public Container;
enum class DividerOrientation { kHorizontal, kVertical };
```

一条细分割线。头文件：`<neoflux/widgets/divider.h>`。

| 方法 | 签名 | 说明 |
|------|------|------|
| 构造 | `Divider()` / `explicit Divider(float thickness)` | |
| `SetColor` | `Divider& SetColor(const Color&)` | **（可链式）** |
| `SetThickness` | `Divider& SetThickness(float)` | 厚度 px，默认 1.0。**（可链式）** |
| `SetOrientation` | `Divider& SetOrientation(DividerOrientation)` | 默认水平。**（可链式）** |

```cpp
std::make_shared<Divider>(1.0F)
    ->SetColor({220, 220, 220, 255})
    .build();
```

---

## Padding 内边距

```cpp
class Padding : public Container;
```

单子节点组件，按固定边距内缩子节点。头文件：`<neoflux/widgets/padding.h>`。
无 setter；子节点与边距在构造时给出。

| 方法 | 签名 |
|------|------|
| 构造 | `explicit Padding(float all, std::shared_ptr<Widget> child = nullptr)` |
| 构造 | `Padding(const EdgeInsets& insets, std::shared_ptr<Widget> child = nullptr)` |

```cpp
std::make_shared<Padding>(16.0F, label);  // 四边统一 16px
```

---

## Center 居中

```cpp
class Center : public Container;
```

撑满父布局并在两轴居中（flex-grow=1，居中对齐）。头文件：
`<neoflux/widgets/center.h>`。

| 方法 | 签名 |
|------|------|
| 构造 | `explicit Center(std::shared_ptr<Widget> child = nullptr)` |

```cpp
std::make_shared<Center>(spinner);
```

---

## Align 对齐

```cpp
class Align : public Container;
```

类似 `Center`，但可指定任意角落/对齐。`Align(kCenter, kCenter)` 等价于
`Center`。头文件：`<neoflux/widgets/align.h>`。

| 方法 | 签名 | 说明 |
|------|------|------|
| 构造 | `Align(HAlign h, VAlign v, std::shared_ptr<Widget> child = nullptr)` | |
| `SetHorizontal` | `Align& SetHorizontal(HAlign)` | **（可链式）** |
| `SetVertical` | `Align& SetVertical(VAlign)` | **（可链式）** |

```cpp
std::make_shared<Align>(HAlign::kRight, VAlign::kTop, closeButton);
```

---

## Card 卡片

```cpp
class Card : public Container;
```

圆角、带内边距的表面面板，包裹单子节点。设计参考 EUI-NEO（Apache-2.0）。
头文件：`<neoflux/widgets/card.h>`。

| 方法 | 签名 | 说明 |
|------|------|------|
| 构造 | `explicit Card(std::shared_ptr<Widget> child = nullptr)` | |
| `SetCardColor` | `Card& SetCardColor(const Color&)` | **（可链式）** |
| `SetCardRadius` | `Card& SetCardRadius(float)` | 圆角 px。**（可链式）** |
| `SetCardPadding` | `Card& SetCardPadding(float)` | 统一内边距。**（可链式）** |

```cpp
std::make_shared<Card>(body)
    ->SetCardColor({255, 255, 255, 255})
    .SetCardRadius(10.0F)
    .SetCardPadding(14.0F)
    .build();
```

---

## ProgressIndicator 进度条

```cpp
class ProgressIndicator : public Widget;
```

确定性水平进度条（圆角轨道 + 已填充部分）。参考 EUI-NEO（Apache-2.0）。
头文件：`<neoflux/widgets/progress_indicator.h>`。

| 方法 | 签名 | 说明 |
|------|------|------|
| 构造 | `ProgressIndicator()` | |
| `SetValue` | `ProgressIndicator& SetValue(float)` | 钳制到 [0,1]。**（可链式）** |
| `GetValue` | `float GetValue() const noexcept` | 当前钳制后的值。 |
| `SetTrackColor` | `ProgressIndicator& SetTrackColor(const Color&)` | 背景轨道。**（可链式）** |
| `SetFillColor` | `ProgressIndicator& SetFillColor(const Color&)` | 已填充部分。**（可链式）** |
| `SetThickness` | `ProgressIndicator& SetThickness(float)` | 轨道高度 px。**（可链式）** |

```cpp
bar->SetValue(0.6F).SetFillColor({40, 140, 255, 255});
```

---

## Switch 开关

```cpp
class Switch : public Widget;
using OnChanged = std::function<void(bool checked)>;
```

胶囊开关，点击切换状态并触发回调。参考 EUI-NEO（Apache-2.0）。
头文件：`<neoflux/widgets/switch.h>`。

| 方法 | 签名 | 说明 |
|------|------|------|
| 构造 | `Switch()` / `explicit Switch(bool checked)` | |
| `SetChecked` | `Switch& SetChecked(bool)` | 程序化设置，不触发回调。**（可链式）** |
| `IsChecked` | `bool IsChecked() const noexcept` | |
| `SetOnChanged` | `Switch& SetOnChanged(OnChanged)` | 用户切换时触发。**（可链式）** |
| `SetLabel` | `Switch& SetLabel(std::string)` | 尾部标签。**（可链式）** |
| `SetOnColor` | `Switch& SetOnColor(const Color&)` | 开启时轨道色。**（可链式）** |
| `SetOffColor` | `Switch& SetOffColor(const Color&)` | 关闭时轨道色。**（可链式）** |

```cpp
auto sw = std::make_shared<Switch>(false);
sw->SetLabel("Dark mode")
  .SetOnChanged([](bool on) { /* ... */ });
```

---

## Checkbox 复选框

```cpp
class Checkbox : public Widget;
using OnChanged = std::function<void(bool checked)>;
```

勾选/未勾选方框，带可选尾部标签。由于命令缓冲只暴露矩形，勾选标记用实心方块绘制。
参考 EUI-NEO（Apache-2.0）。头文件：`<neoflux/widgets/checkbox.h>`。

| 方法 | 签名 | 说明 |
|------|------|------|
| 构造 | `Checkbox()` / `explicit Checkbox(bool checked)` | |
| `SetChecked` | `Checkbox& SetChecked(bool)` | 程序化设置，不触发回调。**（可链式）** |
| `IsChecked` | `bool IsChecked() const noexcept` | |
| `SetOnChanged` | `Checkbox& SetOnChanged(OnChanged)` | 切换时触发。**（可链式）** |
| `SetLabel` | `Checkbox& SetLabel(std::string)` | 尾部标签。**（可链式）** |
| `SetCheckedColor` | `Checkbox& SetCheckedColor(const Color&)` | 勾选时方框色。**（可链式）** |

```cpp
auto cb = std::make_shared<Checkbox>();
cb->SetLabel("Remember me")
  .SetOnChanged([](bool on) { /* ... */ });
```
