# RenderCommand

渲染命令结构体，描述 Render 层需要执行的绘制操作。

## 概述

`RenderCommand` 是 Application 层与 Render 层之间的通信单元。每个命令包含类型和对应的参数。

## 命令类型

| 类型 | 说明 |
|------|------|
| `kNoop` | 空操作 |
| `kBeginFrame` | 开始新帧 |
| `kEndFrame` | 结束帧，提交并交换缓冲区 |
| `kDrawRect` | 绘制矩形 |
| `kDrawRoundedRect` | 绘制圆角矩形 |
| `kDrawText` | 绘制文本（UTF-8） |
| `kClipRect` | 设置裁剪区域 |
| `kTranslate` | 平移变换 |
| `kSave` | 保存渲染状态 |
| `kRestore` | 恢复渲染状态 |

## 工厂方法

| 方法 | 说明 |
|------|------|
| `MakeBeginFrame()` | 创建开始帧命令 |
| `MakeEndFrame()` | 创建结束帧命令 |
| `MakeDrawRect(rect, color)` | 创建绘制矩形命令 |
| `MakeDrawRoundedRect(rect, color, radius)` | 创建绘制圆角矩形命令 |
| `MakeDrawText(text, pos, color, size, font)` | 创建绘制文本命令 |
| `MakeClipRect(rect)` | 创建裁剪命令 |
| `MakeTranslate(x, y)` | 创建平移命令 |
| `MakeSave()` | 创建保存状态命令 |
| `MakeRestore()` | 创建恢复状态命令 |

## 另见

- [RenderContext](./render-context)
- [渲染管线指南](../guide/rendering)
