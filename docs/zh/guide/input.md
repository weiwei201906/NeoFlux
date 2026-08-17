# 输入与事件

鼠标/触摸事件从平台桥接流经 Widget 树。

## 事件流

1. `GlfwBridge` 接收 GLFW 鼠标事件（按钮、移动、滚轮）并通过回调转发
2. `Application` 执行递归 `HitTest()` 找到光标下最深层的 Widget
3. 调用命中 Widget 的事件处理函数，传入局部坐标

## 事件类型

| 事件 | 说明 |
|------|------|
| `OnPointerDown(pos)` | 指针按下 |
| `OnPointerUp(pos)` | 指针释放 |
| `OnPointerMove(pos)` | 指针移动（悬停或拖拽） |
| `OnPointerEnter()` | 指针进入组件区域 |
| `OnPointerExit()` | 指针离开组件区域 |

## 命中测试缓存

指针移动事件频率高，使用命中测试缓存避免每次都遍历整棵树。布局变化时缓存自动失效。

## 拖拽

`Draggable` 组件重写 `OnPointerMove` 更新拖拽偏移。`ScrollView` 支持拖拽滚动。

:::tip
拖拽进行中（`OnPointerDown` 后的移动事件），移动事件会直接发给被按下的 Widget，跳过命中测试。这确保 `Draggable` 在指针移出其布局边界时（因绘制时平移）仍能持续接收事件。
:::

## 下一步

- [Widget 系统](./widgets)
- [路由导航](./routing)
