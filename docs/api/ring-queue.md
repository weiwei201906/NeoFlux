# RingQueue

Lock-free single-producer, single-consumer (SPSC) ring queue for passing
`RenderCommand`s from the application layer to the render layer.

## Header

```cpp
#include <neoflux/core/ring_queue.h>
```

## Template

```cpp
template <typename T>
class SpscRingQueue;
```

## Construction

```cpp
SpscRingQueue<RenderCommand> queue(2048);  // capacity rounded up to power of 2
```

The capacity is rounded up to the next power of 2 via `std::bit_ceil`.

## Methods

### `TryPush()`

```cpp
bool TryPush(const T& value);
bool TryPush(T&& value);
```

Pushes an element. Returns `false` if the queue is full.

### `TryPop()`

```cpp
bool TryPop(T& out);
```

Pops an element into `out`. Returns `false` if the queue is empty.

### `Size()` / `Empty()` / `Capacity()`

```cpp
[[nodiscard]] std::size_t Size() const noexcept;
[[nodiscard]] bool Empty() const noexcept;
[[nodiscard]] std::size_t Capacity() const noexcept;
```

## Thread Safety

- Single producer thread (application layer).
- Single consumer thread (render layer).
- Head and tail counters are cache-line aligned to avoid false sharing.
- Uses `std::construct_at` / `std::destroy_at` for safe element construction
  (supports non-trivially-copyable types like `std::string`).

## Example

```cpp
SpscRingQueue<int> queue(64);

// Producer thread
queue.TryPush(42);

// Consumer thread
int value;
if (queue.TryPop(value)) {
  LOG(INFO) << "Got: " << value;
}
```

## Configuration

Queue capacity is configurable via `--render_queue_capacity` (default 2048).
