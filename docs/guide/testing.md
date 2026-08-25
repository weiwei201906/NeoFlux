# Testing

NeoFlux uses [GoogleTest](https://github.com/google/googletest) for unit
testing.

## Building Tests

Tests are not built by default. Enable them with the `NEOFLUX_BUILD_TESTS`
CMake option:

```bash
mkdir build && cd build
cmake .. -DNEOFLUX_BUILD_TESTS=ON
cmake --build . -j
```

## Running Tests

```bash
cd build
ctest --output-on-failure
```

Or run individual test binaries:

```bash
./bin/ring_queue_test
./bin/ring_queue_stress_test
./bin/widget_test
./bin/route_registry_test
./bin/render_command_test
```

## Test Suites

### Ring Queue Unit Tests (`ring_queue_test`)

8 tests covering:

- Initially empty state
- Push and pop single elements
- Pop from empty returns false
- Fill to capacity
- FIFO ordering
- Wrap-around behavior
- Move-only type support
- Single producer / single consumer concurrency

### Ring Queue Stress Tests (`ring_queue_stress_test`)

6 stress tests designed to catch race conditions, memory corruption, and
integer overflow bugs that unit tests might miss:

| Test | Description | Scale |
|------|-------------|-------|
| `FifoOrderingMultiThread` | Producer pushes 0..N-1, consumer verifies strict ordering | 1M items |
| `NoLossNoDuplicate` | Checksum verify: sum of pushed == sum of popped | 1M items |
| `MinimumCapacity` | Capacity=2 holds exactly 1 element | Boundary |
| `FullEmptyOscillation` | Capacity=4, rapid full/empty transitions | 100K ops |
| `MoveOnlyTypeStress` | Move-only items, live_count==0 at end (no leak/double-free) | 50K items |
| `LargeCapacity` | 1M slot allocation + 10K fill/drain | 1M slots |

:::tip
Stress tests use `std::atomic` counters and checksums to detect subtle
concurrency bugs. Run them with `-DNEOFLUX_BUILD_TESTS=ON` and a Debug build
for maximum assertion coverage.
:::

### Widget Tests

Tests widget layout and properties:

- Container fixed-size layout
- Container wrapping child content
- Widget hierarchy (parent/child)
- Build dirty flag
- Button press state

### Route Registry Tests

Tests route registration and lookup:

- Register and retrieve routes
- Unknown route handling
- Route replacement

### Render Command Tests

Tests render command creation:

- Draw rect commands
- Draw text commands
- Begin/end frame commands
- Transform and clip commands

### Backpressure Stress Tests

Tests the render command queue overflow mechanism. When the UI thread
produces commands faster than the render thread consumes them, the SPSC
queue fills up and `RenderLayer::Submit` truncates the batch (silent frame
drop) to prevent unbounded memory growth.

| Test | Scenario | Verification |
|------|----------|-------------|
| `SingleFrameOverflow` | 10000 commands into 256-cap queue | Exactly 255 submitted, no crash |
| `RepeatedOverflowCycles` | 100 cycles of overflow + drain | No corruption, counts preserved |
| `PartialFillThenOverflow` | 50 pre-filled + 1000 overflow | 50 + 205 = 255 total |
| `LargeQueueNoOverflow` | 4096 queue, 500 commands | All 500 accepted, zero drops |
| `ConcurrentProducerConsumerOverflow` | 16-cap queue, 200ms concurrent | Zero lost items under contention |

The `SimulateSubmit()` helper replicates `RenderLayer::Submit`'s exact
`TryPush` + `break` logic to test the backpressure mechanism in isolation
without a running render thread.

:::tip
Backpressure is a safety mechanism, not a performance target. If your app
triggers queue overflow regularly, reduce per-frame command count (e.g. batch
text draws, cull off-screen widgets) rather than increasing queue capacity.
:::

## Writing Tests

Create a new test file in `tests/`:

```cpp
#include <gtest/gtest.h>
#include <neoflux/widget/container.h>

using namespace neoflux;

TEST(MyWidgetTest, BasicFunctionality) {
  Container container;
  container.SetWidth(100.0F).SetHeight(50.0F);
  EXPECT_FLOAT_EQ(container.GetWidth(), 100.0F);
  EXPECT_FLOAT_EQ(container.GetHeight(), 50.0F);
}
```

Add the test to `tests/CMakeLists.txt`:

```cmake
add_executable(my_test my_test.cpp)
target_link_libraries(my_test PRIVATE neoflux gtest_main)
add_test(NAME MyTest COMMAND my_test)
```

## CI/CD

For continuous integration, run:

```bash
cmake -B build -DNEOFLUX_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
cd build && ctest --output-on-failure
```

## clang-tidy

In addition to unit tests, NeoFlux uses clang-tidy for static analysis:

```bash
clang-tidy -p build src/**/*.cpp
```

The `.clang-tidy` configuration enforces Google C++ style, modern C++
practices, and bug-prone pattern detection.
