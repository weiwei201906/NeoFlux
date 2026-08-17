# RenderContext

渲染上下文，用于录制渲染命令。

## 概述

`RenderContext` 管理渲染命令列表和变换/裁剪栈。Application 层通过 RenderContext 录制命令，然后提交给 Render 层执行。

## 方法

| 方法 | 说明 |
|------|------|
| `Clear()` | 清空所有命令 |
| `AppendCommand(cmd)` | 追加渲染命令 |
| `GetCommands() -> const vector&` | 获取命令列表 |
| `Save()` | 保存当前变换/裁剪状态 |
| `Restore()` | 恢复上一个变换/裁剪状态 |
| `Translate(x, y)` | 平移变换 |
| `ClipRect(rect)` | 设置裁剪矩形 |
| `DrawRect(rect, color)` | 绘制矩形 |
| `DrawRoundedRect(rect, color, radius)` | 绘制圆角矩形 |
| `DrawText(text, pos, color, size, font)` | 绘制文本 |

## 另见

- [RenderCommand](./render-command)
- [渲染管线指南](../guide/rendering)
