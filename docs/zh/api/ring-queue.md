# RingQueue

SPSC（单生产者单消费者）无锁环形队列。

## 概述

`SpscRingQueue<T>` 是无锁的有界环形缓冲区，用于 Application 层与 Render 层之间的命令传递。

## 特性

- 无锁，使用原子操作和内存序
- 容量运行时可配置，自动向上取整为 2 的幂
- 位运算回绕，缓存行对齐 head/tail
- 单生产者单消费者，无需互斥锁

## 基本用法

```cpp
#include <neoflux/core/ring_queue.h>

neoflux::SpscRingQueue<int> queue(1024);  // 容量自动取整为 1024

// 生产者线程
queue.TryPush(42);

// 消费者线程
int value;
if (queue.TryPop(value)) {
  // 处理 value
}
```

## 方法

| 方法 | 说明 |
|------|------|
| `TryPush(value) -> bool` | 尝试入队，队列满时返回 false |
| `TryPop(value) -> bool` | 尝试出队，队列空时返回 false |
| `Size() -> size_t` | 当前队列大小 |
| `Capacity() -> size_t` | 队列容量 |
| `IsEmpty() -> bool` | 队列是否为空 |
| `IsFull() -> bool` | 队列是否已满 |

## 另见

- [架构指南](../guide/architecture)
- [RenderCommand](./render-command)
