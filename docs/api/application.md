# Application

The main application class. Owns the window, event loop, render layer, and
navigation stack.

## Header

```cpp
#include <neoflux/app/application.h>
```

## Construction

```cpp
Application app;
```

## Methods

### `Init()`

```cpp
bool Init(int argc, char** argv, int width, int height,
          std::string_view title);
```

Initializes the application. Parses gflags, initializes glog, creates the
render layer and window. Returns `true` on success.

### `Run()`

```cpp
void Run();
```

Runs the event loop. Blocks until `Stop()` is called or the window is closed.

### `Stop()`

```cpp
void Stop();
```

Stops the event loop and render layer.

### `PushRoute()`

```cpp
void PushRoute(std::string_view route_name);
```

Builds and displays a registered route. The previous route remains on the
stack.

### `PopRoute()`

```cpp
void PopRoute();
```

Returns to the previous route on the stack.

### `ReplaceRoute()`

```cpp
void ReplaceRoute(std::string_view route_name);
```

Replaces the current route without adding to the stack.

### `GetRootWidget()`

```cpp
Widget* GetRootWidget() const;
```

Returns the current root widget (top of the navigation stack).

### `MarkFrameDirty()`

```cpp
void MarkFrameDirty();
```

Triggers a layout/paint cycle on the next frame.

### `GetEventLoop()`

```cpp
EventLoop& GetEventLoop();
```

Returns the event loop reference (for scheduling coroutines).

## Example

```cpp
int main(int argc, char** argv) {
  RouteRegistry::Instance().RegisterRoute("/", BuildHome);

  Application app;
  if (!app.Init(argc, argv, 400, 300, "My App")) {
    return 1;
  }
  app.PushRoute("/");
  app.Run();
  return 0;
}
```
