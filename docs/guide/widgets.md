# Widget System

NeoFlux uses a Flutter-style widget system. UIs are built by composing widget
objects into a tree. Each widget overrides virtual functions to customize its
behavior.

## Widget Base Class

`Widget` is the abstract base class for all UI elements. Key virtual functions:

| Function | Purpose |
|----------|---------|
| `Build(BuildContext&)` | Returns child widget(s) for stateful widgets |
| `OnMeasure(width, mode, height, mode)` | Returns intrinsic size for leaf widgets |
| `Paint(RenderContext&)` | Generates render commands |
| `HitTest(Point)` | Tests if a point hits this widget |
| `OnPointerDown(const Point&)` | Handles press event (returns true if consumed) |
| `OnPointerUp(const Point&)` | Handles release event |
| `OnPointerMove(const Point&)` | Handles pointer move (hover/drag) |
| `OnPointerEnter()` | Handles pointer entering widget bounds |
| `OnPointerExit()` | Handles pointer leaving widget bounds |
| `OnPointerScroll(const Point&, x, y)` | Handles scroll events |
| `GetWidgetName()` | Returns the widget's name for debugging (string_view) |
| `GetPaintOffset()` | Returns paint-time transform offset (default zero) |

:::tip
`GetPaintOffset()` is virtual. Widgets that apply a paint-time transform
(e.g. `Draggable` with its drag offset) **must** override it so that event
dispatch correctly converts visual coordinates to widget-local coordinates.
Without this, clicks at the widget's visual position will miss because
`HitTest` and `local_pos` calculation use layout coordinates.
:::

## Paint-Time Transforms and Hit Testing

Widgets may apply transforms at paint time (e.g. `Draggable` translates by
`drag_offset_`). These transforms do **not** affect Taitank layout (`bounds_`
stays at the layout position). To keep hit testing and event coordinates
consistent with the visual position:

1. **`GetPaintOffset()`** returns the visual offset (override in subclasses).
2. **`HitTest(Point)`** subtracts the paint offset before delegating to the
   base class, converting visual click coordinates to layout coordinates.
3. **Event dispatch** (`DispatchPointerEvent`/`DispatchPointerMove`) subtracts
   `GetPaintOffset()` when computing `local_pos`, so handlers receive
   coordinates relative to the visual top-left.

```
Visual position  = Layout position + GetPaintOffset()
local_pos        = cursor_global - visual_position
```

## Draggable

`Draggable` is a container whose children follow the pointer during a drag.
The widget **center always follows the cursor** — on press it jumps to center
on the click point, and during movement it stays centered.

```cpp
auto drag = std::make_shared<Draggable>();
drag->AddChild(box);  // any widget tree
parent->AddChild(drag);
```

**How it works:**
- `OnPointerDown`: sets `dragging_ = true` and immediately adjusts
  `drag_offset_` so the widget center lands on the cursor.
- `OnPointerMove`: `drag_offset_ += local_pos - bounds.size / 2`, keeping
  the center on the cursor. No `MarkNeedsBuild()` — the offset is applied
  at paint time only, so only `MarkFrameDirty()` (called by event dispatch)
  is needed.
- `OnPointerUp`: ends the drag, returns to `kIdle` state.
- `Paint`: `context.Translate(drag_offset_)` before painting children.
- `GetPaintOffset`: returns `drag_offset_` for correct hit testing.
- `HitTest`: overridden to subtract `drag_offset_` before base-class hit test.

:::warning
`Draggable` uses paint-time translation, not layout changes. The widget's
`bounds_` (layout position) never changes during a drag. This means sibling
widgets do not reflow, and the drag is $O(1)$ per frame — no Taitank relayout.
:::

## Widget Lifecycle

```
Create → Build → Layout → Paint → Display
                ↑                    │
                └──── dirty flag ────┘
```

1. **Create**: Widget is constructed and added to the tree.
2. **Build**: `Build()` is called for dirty widgets to create/update children.
3. **Layout**: Taitank computes positions and sizes.
4. **Paint**: `Paint()` generates render commands.
5. **Display**: Render layer draws the frame.

When a widget calls `MarkNeedsBuild()`, it is rebuilt on the next frame. When
the application calls `MarkFrameDirty()`, a full layout/paint cycle is triggered.

## Container

`Container` is the primary layout widget. It maps directly to a Taitank flex
node and supports:

- Flex direction (row/column)
- Justify content (start/center/end/space-between/space-around)
- Align items (start/center/end/stretch)
- Padding and margin
- Background color
- Border radius
- Flex grow/shrink

```cpp
auto col = std::make_shared<Container>();
col->SetFlexDirection(FlexDirection::kColumn)
   .SetJustifyContent(HAlign::kCenter)
   .SetAlignItems(VAlign::kCenter)
   .SetPadding({.left = 16, .top = 16, .right = 16, .bottom = 16})
   .SetBackgroundColor({.r = 255, .g = 255, .b = 255, .a = 255})
   .SetBorderRadius(8.0F);
```

## Text

`Text` renders a single-line string. It is a leaf widget that reports its
intrinsic size via `OnMeasure()`.

```cpp
auto label = std::make_shared<Text>("Hello World");
label->SetFontSize(18.0F)
     .SetTextColor({.r = 0, .g = 0, .b = 0, .a = 255})
     .SetAlignment(HAlign::kCenter)
     .SetFont("NotoSansSC-Regular");  // optional: font by name
```

## Button

`Button` is a clickable widget with a label and press callback.

```cpp
auto btn = std::make_shared<Button>("Click Me");
btn->SetOnPressed([]() {
    LOG(INFO) << "Button pressed!";
});
btn->SetFontSize(16.0F)
   .SetBackgroundColor({.r = 33, .g = 150, .b = 243, .a = 255})
   .SetTextColor({.r = 255, .g = 255, .b = 255, .a = 255})
   .SetBorderRadius(6.0F);
```

## ScrollView

`ScrollView` provides a scrollable viewport. Content larger than the viewport
can be panned via mouse wheel (desktop) or touch drag (mobile).

```cpp
auto scroll = std::make_shared<ScrollView>();

auto content = std::make_shared<Container>();
content->SetFlexDirection(FlexDirection::kColumn);
for (int i = 0; i < 20; ++i) {
  auto item = std::make_shared<Text>("Item " + std::to_string(i));
  content->AddChild(item);
}

scroll->SetContent(content);
parent->AddChild(scroll);
```

## Expanded

`Expanded` is a convenience widget that sets `flex_grow` to fill remaining
space in a flex parent.

```cpp
auto row = std::make_shared<Container>();
row->SetFlexDirection(FlexDirection::kRow);

auto left = std::make_shared<Expanded>(std::make_shared<Text>("Left"));
auto right = std::make_shared<Expanded>(std::make_shared<Text>("Right"), 2);
row->AddChild(left);
row->AddChild(right);  // takes 2x the space of left
```

## SizedBox

`SizedBox` is a container with explicit width and height. Useful for fixed-size
spacing or dimensions.

```cpp
auto spacer = std::make_shared<SizedBox>(0, 16);  // 16px vertical spacer
parent->AddChild(spacer);

auto card = std::make_shared<SizedBox>(200, 120);
card->SetBackgroundColor({.r = 240, .g = 240, .b = 240, .a = 255});
```

## StatefulWidget

`StatefulWidget` holds mutable state that can trigger rebuilds. Override
`Build()` to return the widget tree based on current state.

```cpp
class CounterWidget : public StatefulWidget {
 public:
  std::shared_ptr<Widget> Build(BuildContext& context) override {
    auto col = std::make_shared<Container>();
    auto label = std::make_shared<Text>("Count: " + std::to_string(count_));
    auto btn = std::make_shared<Button>("Increment");
    btn->SetOnPressed([this]() {
      count_++;
      MarkNeedsBuild();
    });
    col->AddChild(label);
    col->AddChild(btn);
    return col;
  }
 private:
  int count_ = 0;
};
```

Call `MarkNeedsBuild()` when state changes to schedule a rebuild.

## TextField

A single-line editable text input widget. Supports cursor navigation,
placeholder text, UTF-8 insertion/deletion, and focus management.

```cpp
auto field = std::make_shared<TextField>();
field->SetPlaceholder("Enter your name...");
field->SetFontSize(16.0F);
field->SetOnSubmit([](std::string_view text) {
  LOG(INFO) << "Submitted: " << text;
});
field->SetOnChange([](std::string_view text) {
  // Called on every keystroke.
});
container->AddChild(field);
```

### Keyboard Navigation

| Key | Action |
|-----|--------|
| Left/Right | Move cursor one character |
| Home/End | Move cursor to start/end |
| Backspace | Delete character before cursor |
| Delete | Delete character after cursor |
| Enter | Trigger `OnSubmit` callback |
| Ctrl+A | Move cursor to end (select all) |

Click the field to acquire keyboard focus. The cursor blinks at ~1Hz.
TextField is fully UTF-8 aware: multi-byte characters are treated as a
single unit for cursor movement and deletion.

:::tip
TextField must call `EnableMeasureFunction()` in its constructor so
Taitank knows its intrinsic size. Without it, the widget has zero bounds
and hit-testing will fail (click does nothing).
:::

## MediaWidget

A media playback widget backed by the **ffplay** subprocess (from FFmpeg).
Launches ffplay as a child process to decode and render audio/video. This
keeps the framework lightweight — no FFmpeg libraries are linked, and the
same code works on Windows, Linux, and macOS.

```cpp
auto media = std::make_shared<MediaWidget>();
media->SetSource("video.mp4");
media->SetExtraArgs("-vcodec h264 -acodec aac -fs");
media->Play();
container->AddChild(media);
```

### FFplay Configuration

- **Default path**: `"ffplay"` (resolved from `PATH` at runtime)
- **Compile-time path**: `-DNEOFLUX_FFPLAY_PATH=/usr/bin/ffplay`
- **Runtime path**: `media->SetFfplayPath("/custom/ffplay")`
- **Extra args**: `media->SetExtraArgs("-fs -autoexit")` — appended after
  the default `-autoexit` flag, before the source URL

### Requirements

ffplay must be installed and available on `PATH` (or configured via
`NEOFLUX_FFPLAY_PATH`):

- **Windows**: Download from [gyan.dev](https://www.gyan.dev/ffmpeg/builds/)
  and add `bin/` to `PATH`
- **Linux**: `sudo apt install ffmpeg`
- **macOS**: `brew install ffmpeg`

:::warning
MediaWidget uses subprocess isolation, so FFmpeg's GPL/LGPL license does
not propagate to the NeoFlux framework binary. The framework itself
remains GPL-3.0.
:::

### Test Video

Generate a 10-second test pattern video with audio:

```bash
# Linux/macOS
bash examples/media_demo/generate_test_video.sh

# Windows
examples\media_demo\generate_test_video.bat
```

This creates `examples/media_demo/test_video.mp4` using ffmpeg's
`testsrc` and `sine` filters.
