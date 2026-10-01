# Widget System

NeoFlux uses a Flutter-style widget model: UIs are built by composing
`shared_ptr<Widget>` nodes into a tree, and layout is computed by the Taitank
flexbox engine. This page covers the widget set that ships today and how the
widget state machine interacts with C++20 coroutines.

## The widget set

| Widget | Base | Purpose |
|--------|------|---------|
| `Widget` | — | Abstract base. Owns a Taitank node, bounds, children, and lifecycle state. |
| `Container` | `Widget` | Flexbox box: flex direction, justify/align, padding, margin, background, border radius. |
| `Text` | `Widget` | Leaf; renders one UTF-8 string with a chosen font, size, color, alignment. |
| `Button` | `Widget` | Clickable label with an `on_pressed` callback and pressed styling. |
| `ScrollView` | `Widget` | Clipped, scrollable viewport; pans content by wheel or drag. |
| `SizedBox` | `Container` | Forces fixed width/height (0/negative = auto on that axis). |
| `Expanded` | `Container` | Flex child that grows to fill remaining main-axis space. |
| `Draggable` | `Container` | Box that follows the pointer; offset applied at paint time so layout is unaffected. |
| `RouteRegistry` | — | Singleton map of route name → widget builder; drives navigation. |

## Widget lifecycle

```
Create -> Build -> Layout (Taitank) -> Paint (record commands) -> Render thread draws
             ^                                                          |
             |__________________ MarkNeedsBuild() / MarkFrameDirty() ____|
```

1. **Build** — dirty widgets return a new subtree from `Build(BuildContext&)`.
2. **Layout** — the root `PerformLayout(width, height)` runs Taitank and
   copies computed rectangles back into every widget.
3. **Paint** — each widget records draw commands into the `RenderContext`.
4. **Submit** — commands cross to the render thread through the SPSC queue.

`MarkNeedsBuild()` marks one subtree for rebuild; `MarkFrameDirty()` forces a
layout/paint cycle for the whole frame.

## Container

The primary layout widget, backed one-to-one by a Taitank flex node:

```cpp
auto col = std::make_shared<Container>();
col->SetFlexDirection(FlexDirection::kColumn)
   .SetJustifyContent(HAlign::kCenter)
   .SetAlignItems(VAlign::kCenter)
   .SetPadding({.left = 16, .top = 16, .right = 16, .bottom = 16})
   .SetBackgroundColor({.r = 245, .g = 245, .b = 250, .a = 255})
   .SetBorderRadius(8.0F);
```

Chaining setters return `Container&`, so you can fluently configure a box.

## Text

A leaf widget that reports its own intrinsic size via `OnMeasure()`:

```cpp
auto label = std::make_shared<Text>("Hello World");
label->SetFontSize(18.0F)
     .SetTextColor({.r = 0, .g = 0, .b = 0, .a = 255})
     .SetAlignment(HAlign::kCenter)
     .SetFont("NotoSansSC-Regular");  // optional, by font stem
```

## Button

```cpp
auto btn = std::make_shared<Button>("Click Me");
btn->SetOnPressed([]() { LOG(INFO) << "pressed"; });
btn->SetFontSize(16.0F)
   .SetBackgroundColor({.r = 33, .g = 150, .b = 243, .a = 255})
   .SetTextColor({.r = 255, .g = 255, .b = 255, .a = 255});
```

`Button` tracks press/release itself and only fires `on_pressed` on a real
press-release over the widget.

## ScrollView

Set a single content child; anything larger than the viewport is clipped and
panned by the mouse wheel or drag:

```cpp
auto scroll = std::make_shared<ScrollView>();

auto content = std::make_shared<Container>();
content->SetFlexDirection(FlexDirection::kColumn);
for (int i = 0; i < 20; ++i) {
  content->AddChild(std::make_shared<Text>("Item " + std::to_string(i)));
}

scroll->SetContent(content);
parent->AddChild(scroll);
```

## SizedBox

Fixed-size box (use 0 on an axis to leave it auto-sized):

```cpp
parent->AddChild(std::make_shared<SizedBox>(0.0F, 16.0F));  // 16px gap

auto card = std::make_shared<SizedBox>(200.0F, 120.0F);
card->SetBackgroundColor({.r = 240, .g = 240, .b = 240, .a = 255});
```

## Expanded

Fills the remaining main-axis space in a flex parent (equivalent to
`Container` with `flex_grow = flex`):

```cpp
auto row = std::make_shared<Container>();
row->SetFlexDirection(FlexDirection::kRow);

row->AddChild(std::make_shared<Expanded>(std::make_shared<Text>("Left")));
row->AddChild(std::make_shared<Expanded>(std::make_shared<Text>("Right"), /*flex=*/2));
// "Right" takes twice the free space of "Left".
```

## Draggable

A `Container` whose contents follow the pointer. The drag offset is applied at
paint time (a translate transform), so layout bounds stay put — widgets under
the box do not jump. Observe `OnStateChanged` to react to
`kIdle` / `kHovering` / `kDragging`.

## RouteRegistry and navigation

Register a builder per route name at startup, then push/pop a navigation
stack:

```cpp
RouteRegistry::Instance().RegisterRoute("/home", BuildHomePage);
RouteRegistry::Instance().RegisterRoute("/settings", BuildSettingsPage);

app.PushRoute("/settings");  // builds and shows the settings page
app.PopRoute();              // returns to the previous route (refuses to pop root)
```

Inside a widget, use the `BuildContext`:

```cpp
Application* app = context.GetApplication();
nav_button->SetOnPressed([app] { app->PushRoute("/settings"); });
```

## State machine + coroutine `Sleep`

Widgets carry a lightweight `WidgetState` set via `SetState()`. The built-in
states are:

| State | Meaning |
|-------|---------|
| `kIdle` | Resting, no interaction. |
| `kHovering` | Pointer over the widget. |
| `kDragging` | Pointer captured and moving. |
| `kLoading` | Busy / in progress. |
| `kSuccess` | Operation finished. |

`OnStateChanged(from, to)` is your hook to launch animations or repaint.

Because NeoFlux is single-threaded on the UI side, "waiting" is done with
C++20 coroutines rather than threads. `co_await Sleep(duration)` suspends the
coroutine on the event loop's timer queue and resumes on the UI thread after
the delay — no callback chains, no data races:

```cpp
#include <neoflux/core/task.h>

neoflux::Task<void> LongPressDetector(std::weak_ptr<Widget> weak) {
  co_await neoflux::Sleep(std::chrono::milliseconds(500));
  auto w = weak.lock();
  if (!w) co_return;                       // widget gone
  if (w->GetState() == WidgetState::kDragging) {
    // still held after 500 ms -> long press confirmed
  }
}
```

::: tip State machine as a condition lock
Launch the coroutine on pointer-down, then `Sleep()`. When it wakes, it
**checks the widget state** rather than cancelling anything explicitly. If
the pointer was released early, the state has already changed and the coroutine
exits silently. This is the pattern used by `drag_demo` and `loading_demo`.
See [Coroutines](./coroutines) for the full `Task` / `Yield` / `Sleep` API.
:::
