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
./bin/widget_test
./bin/route_registry_test
./bin/render_command_test
```

## Test Suites

### Ring Queue Tests

Tests the SPSC lock-free ring queue:

- Push/pop single elements
- FIFO ordering
- Capacity overflow behavior
- Concurrent producer/consumer
- Non-trivially-copyable types (std::string)

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
